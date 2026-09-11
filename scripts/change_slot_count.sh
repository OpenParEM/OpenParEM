#!/bin/sh

print_help=0

# check for no arguments
if [ $# -eq 0 ]
then
   print_help=1
fi

if [ $# -eq 1 ]; then
   if [ "$1" = "--help" ]; then
      print_help=1
   fi
fi

if [ ! $# -eq 3 ]
then
   print_help=1
fi

if [ "$print_help" = 1 ]; then
   echo "Usage: change_slot_count.sh directory from to"
   echo "       where directory is the starting directory for making changes"
   echo "             from is an integer existing slot count number"
   echo "             to is an integer changed slot count number"
   echo ""
   echo "Recursively changes the slot count number to adjust projects for smaller or larger machines."
   exit 1
fi

old_text="gui\.slot\.count "$2
new_text="gui.slot.count "$3

root="${1:-.}"

find "$root" -type f -name '*.proj' -print |
while IFS= read -r file
do

   cat $file | grep -q "$old_text"
   if [ $? -eq 0 ]; then
     echo "Modifying: $file"
     sed -i "s/$old_text/$new_text/" "$file"
   fi
done


exit 0
