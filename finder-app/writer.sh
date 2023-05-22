if [ -z "$1" ]
then
  echo "No path supplied"
  exit 1
fi

if [ -z "$2" ]
  then
    echo "No text supplied"
    exit 1
fi

mkdir -p `dirname $1`

echo "$2" > "$1"
result=$(cat "$1")

if [ "$result" != "$2" ]
  then
    echo "Writing file failed"
    exit 1
fi

