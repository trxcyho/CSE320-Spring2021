declare -a files=("rsrc/algebric.ntn" "rsrc/boudy.ntn" "rsrc/keywords.ntn" "rsrc/shortened.ntn")
declare -a arr=()
readarray -t arr < examples.txt
echo $arr
valgrind="valgrind --leak-check=full --show-leak-kinds=all"
command="bin/notation"
path=(echo $PWD)
make clean debug
echo "START"
if [ ! -d $path/result ] 
then
    mkdir $path/result
fi
echo > test-stdout
echo > test-stderr
echo > test-stderr-summary
echo > test-exitcode
for file in "${files[@]}"
do
    echo "Processing: $file"
    for options in "${arr[@]}"
        do
            echo $valgrind $command $file $options
            echo $valgrind $command $file $options >> test-stdout
            echo $command $file $options >> test-stderr
            echo $valgrind $command $file $options >> test-stderr-summary
            echo $valgrind $command $file $options >> test-exitcode
            $valgrind $command $file $options >> test-stdout 2> tmp-test
            echo $? >> test-exitcode
            $command $file $options 2>> test-stderr > /dev/null
            cat tmp-test | grep ERROR | cut -c 10- >> test-stderr-summary
            echo >> test-stderr
        done
done
