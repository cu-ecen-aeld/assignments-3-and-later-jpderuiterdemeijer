if [ -z "$1" ]
then
  echo "No path supplied"
  exit 1
else 
  if [ ! -d "$1" ]
  then
    echo "Path supplied does not exist"
    exit 1
  fi
fi

if [ -z "$2" ]
  then
    echo "No text supplied"
    exit 1
fi

filecount=$(find "$1" -type f | wc -l)
matchcount=$(grep -rwI "$2" "$1" | wc -l)

echo "The number of files are $filecount and the number of matching lines are $matchcount"
