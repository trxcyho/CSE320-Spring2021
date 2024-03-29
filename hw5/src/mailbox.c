#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "server.h"
#include "globals.h"
#include "mailbox.h"
#include <csapp.h>

struct mailbox{
	char *name;
	int ref_count;
	int defunct; //1=true 0=false
	sem_t m_mutex;
	int size;
	MAILBOX_ENTRY **entries;
	MAILBOX_DISCARD_HOOK *hook;
	sem_t counting; //keep track amount of entries
};

/*
 * A mailbox provides the ability for its client to set a hook to be
 * called when an undelivered entry in the mailbox is discarded on
 * mailbox finalization.  For example, the hook might arrange for the
 * sender of an undelivered message to receive a bounce notification.
 * The discard hook is not responsible for actually deallocating the
 * mailbox entry; that is handled by the mailbox finalization procedure.
 * There is one special case that the discard hook must handle, and that
 * is the case that the "from" field of a message is NULL.  This will only
 * occur in a mailbox entry passed to discard hook, and it indicates that
 * the sender of the message was in fact the mailbox that is now being
 * finalized.  Since that mailbox can no longer be used, it does not make
 * sense to do anything further with it (and in fact trying to do so would
 * result in a deadlock because the mailbox is locked), so it has been
 * replaced by NULL.
 *
 * The following is the type of discard hook function.
 */
// void calling_hook(MAILBOX_ENTRY *mb_entry){
// 	if(mb_entry->type == MESSAGE_ENTRY_TYPE && mb_entry->content.message.from != NULL){
// 		MESSAGE msg = mb_entry->content.message;
// 		mb_add_notice(msg.from,BOUNCE_NOTICE_TYPE,msg.msgid);
// 	}
// 	return;
// }

/*
 * Create a new mailbox for a given handle.  A private copy of the
 * handle is made.  The mailbox is returned with a reference count of 1.
 */
MAILBOX *mb_init(char *handle){
	debug("initialize mailbox");
	MAILBOX *mailbox = malloc(sizeof(MAILBOX));
	char *copy_handle = malloc(strlen(handle) + 1);
	strcpy(copy_handle, handle);
	if(mailbox == NULL || copy_handle == NULL)
		return NULL;
	mailbox -> name = copy_handle;
	sem_init(&(mailbox->m_mutex), 0, 1);
	Sem_init(&(mailbox->counting), 0, 0);
	mailbox -> ref_count = 1;
	mailbox -> defunct = 0;
	mailbox-> size = 0;
	mailbox -> entries = malloc(sizeof(MAILBOX_ENTRY *) * 1);
	mailbox -> entries[0] = NULL;
	//initialize hook?
	mailbox -> hook = NULL;
	return mailbox;
}

/*
 * Set the discard hook for a mailbox.
 */
void mb_set_discard_hook(MAILBOX *mb, MAILBOX_DISCARD_HOOK *func){
	debug("in mb_set_discard_hook");
	P(&mb->m_mutex);
	mb-> hook = func;
	V(&mb->m_mutex);
}

/*
 * Increase the reference count on a mailbox.
 * This must be called whenever a pointer to a mailbox is copied,
 * so that the reference count always matches the number of pointers
 * that exist to the mailbox.
 */
void mb_ref(MAILBOX *mb, char *why){
	P(&mb->m_mutex);
	mb -> ref_count = (mb -> ref_count) +1;
	debug("Increase mailbox ref_count (%d -> %d): %s", mb -> ref_count - 1, mb -> ref_count, why);
	V(&mb->m_mutex);
}

/*
 * Decrease the reference count on a mailbox.
 * This must be called whenever a pointer to a mailbox is discarded,
 * so that the reference count always matches the number of pointers
 * that exist to the mailbox.  When the reference count reaches zero,
 * the mailbox will be finalized.
 */
void mb_unref(MAILBOX *mb, char *why){
	P(&mb->m_mutex);
	mb -> ref_count = (mb -> ref_count) -1;
	debug("Decrease mailbox ref_count (%d -> %d): %s", mb -> ref_count + 1, mb -> ref_count, why);
	V(&mb->m_mutex);
	if(mb -> ref_count == 0){
		if(mb-> hook != NULL){
			for(int i = 0; i < mb->size; i++){
				//TODO: call discord hook on each entry
				// user_unref(getmb->handle)
				(mb->hook)(mb->entries[i]);
			}
		}
		debug("finalize mailbox");
		free(mb ->name);
		free(mb->entries);
		free(mb);
		debug("free mailbox");
	}
}

/*
 * Shut down this mailbox.
 * The mailbox is set to the "defunct" state.  A defunct mailbox should
 * not be used to send any more messages or notices, but it continues
 * to exist until the last outstanding reference to it has been
 * discarded.  At that point, the mailbox will be finalized, and any
 * entries that remain in it will be discarded.
 */

void mb_shutdown(MAILBOX *mb){
	debug("in mb_shutdown");

	P(&mb->m_mutex);
	// mb_unref(mb, "for reference discarded for terminating mailbox service");
	mb -> defunct = 1;
	V(&mb->counting);
	V(&mb->m_mutex);

}

/*
 * Get the handle of the user associated with a mailbox.
 * The handle is set when the mailbox is created and it does not change.
  */

char *mb_get_handle(MAILBOX *mb){
	return mb->name;
}

/*
 * Add a message to the end of the mailbox queue.
 *   msgid - the message ID
 *   from - the sender's mailbox
 *   body - the body of the message, which can be arbitrary data, or NULL
 *   length - number of bytes of data in the body
 *
 * The message body must have been allocated on the heap,
 * but the caller is relieved of the responsibility of ultimately
 * freeing this storage, as it will become the responsibility of
 * whomever removes this message from the mailbox.
 *
 * Unless the recipient's mailbox ("mb') is the same as the sender's
 * mailbox ("from"), this function must increment the reference count of the
 * senders mailbox, to ensure that this mailbox persists so that it can receive
 * a notification in case the message bounces.
 */
void mb_add_message(MAILBOX *mb, int msgid, MAILBOX *from, void *body, int length){
	debug("in mb_add_message");
	MESSAGE *message = malloc(sizeof(MESSAGE));
	message->msgid = msgid;
	if(mb != from)
		mb_ref(from, "for newly created message with mailbox as sender");
	message->from = from;
	message->length = length;
	message->body = body;

	MAILBOX_ENTRY *entry = malloc(sizeof(MAILBOX_ENTRY));
	entry -> type = MESSAGE_ENTRY_TYPE;
	entry -> content.message = *message;
	P(&mb -> m_mutex);
	debug("mb_add_message: size=%d", mb-> size);
	mb-> entries = realloc(mb-> entries, sizeof(MAILBOX_ENTRY *) * (mb-> size +2));
	mb -> entries[mb-> size] = entry;
	mb-> size = (mb-> size) +1;
	mb -> entries[mb-> size] = NULL;
	V(&mb->counting);
	if(body != NULL)
		debug("Add message: msgid=%d, length=%d, body=%s, from=%s", msgid, length, (char *)body, mb_get_handle(from));
	else
		debug("Add message: msgid=%d, length=%d, body=NULL, from=%s", msgid, length, mb_get_handle(from));
	V(&mb -> m_mutex);
}

/*
 * Add a notice to the end of the mailbox queue.
 *   ntype - the notice type
 *   msgid - the ID of the message to which the notice pertains
 */
void mb_add_notice(MAILBOX *mb, NOTICE_TYPE ntype, int msgid){
	NOTICE *notice = malloc(sizeof(NOTICE));
	notice -> type = ntype;
	notice -> msgid = msgid;

	MAILBOX_ENTRY *entry = malloc(sizeof(MAILBOX_ENTRY));
	entry -> type = NOTICE_ENTRY_TYPE;
	entry -> content.notice = *notice;

	P(&mb -> m_mutex);
	mb-> entries = realloc(mb-> entries, sizeof(MAILBOX_ENTRY *) * (mb-> size +2));
	mb -> entries[mb-> size] = entry;
	mb-> size = (mb-> size) +1;
	mb -> entries[mb-> size] = NULL;
	V(&mb->counting);
	debug("Add notice: msgid=%d, notice type=%d", msgid, ntype);
	V(&mb -> m_mutex);
}

/*
 * Remove the first entry from the mailbox, blocking until there is
 * one.  The caller assumes the responsibility of freeing the entry
 * and its body, if present.  In addition, if it is a message entry,
 * then the caller must decrease the reference count on the sender's
 * mailbox to account for the destruction of the pointer to it.
 *
 * This function will return NULL in case the mailbox is defunct.
 * The thread servicing the mailbox should use this as an indication
 * that service should be terminated.
 */
MAILBOX_ENTRY *mb_next_entry(MAILBOX *mb){
	//check if something in array
	debug("in mb_next_entry");
	//shift array downwards
	P(&mb->counting);//decrease amount and locks when nothing

	// if(mb->size == 0)
	// 	P(&mb->counting);

	if(mb -> defunct == 1){
		debug("before null in mb_next_entry");
		return NULL;
	}
	P(&mb -> m_mutex);
	debug("num entries in mailbox %d\n", mb->size);
	MAILBOX_ENTRY *first = mb-> entries[0];
	for(int i = 0; i < (mb->size)-1; i++){
		MAILBOX_ENTRY *entry = mb-> entries[i+1];
		mb-> entries[i] = entry;
	}
	mb-> entries = realloc(mb-> entries, sizeof(MAILBOX_ENTRY *) * (mb-> size));
	mb-> size = (mb-> size) -1;
	mb->entries[mb->size] = NULL;
	if(first -> type == MESSAGE_ENTRY_TYPE){
		if(first -> content.message.from != mb)
			mb_unref(first -> content.message.from, "for reference to sender's mailbox held by message being removed");
	}

	V(&mb -> m_mutex);
	return first;
}
