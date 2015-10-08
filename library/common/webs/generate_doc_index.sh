#!/bin/bash

BASE_DIR=$1
INDEX_FILE=$BASE_DIR/index.html
BASE_DIR_LISTING=$BASE_DIR/*
BASENAME=/usr/bin/basename

# Generate the index file from the directory listing of base dir
if [ -e $INDEX_FILE ]; then
    echo "<html>" > $INDEX_FILE
else
    echo "<html>" >> $INDEX_FILE
fi
echo "<head>OpenContrail Message Documentation</head>" >> $INDEX_FILE
echo "<link href=\"/doc-style.css\" rel=\"stylesheet\" type=\"text/css\"/>" >> $INDEX_FILE
echo "<table><tr><th>Module</th></tr>" >> $INDEX_FILE
for file in $BASE_DIR_LISTING; do
    if [ -d $file ]; then
        dirname=$($BASENAME $file)
        echo "<tr><td><a href=$dirname/index.html>$dirname</a></td></tr>" >> $INDEX_FILE
    else
        filename=$($BASENAME $file)
        extension="${filename##*.}"
        # Ignore non HTML files
        if [ $extension != "html" ]; then
            continue
        fi
        filename=$($BASENAME $file .html)
        # Ignore index.html
        if [ $filename == "index" ]; then
            continue
        fi
        echo "<tr><td><a href="$filename".html>$filename</a></td></tr>" >> $INDEX_FILE
    fi
done
echo "</table>" >> $INDEX_FILE
echo "</html>" >> $INDEX_FILE
