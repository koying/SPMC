#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "Source dir of java file to sync required"
  exit 1
fi

SRC_DIR=$1
ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../../../" && pwd )"
TMP_JAVA_DIR="${ROOT_DIR}/tools/android/packaging/tmp_java"
TARGET_JAVA_DIR="${ROOT_DIR}/tools/android/packaging/xbmc/src"

rm -rf $TMP_JAVA_DIR
mkdir $TMP_JAVA_DIR

function cp_java {
  mkdir -p $(dirname "$TMP_JAVA_DIR/$1")
  cp $SRC_DIR/$1 $TMP_JAVA_DIR/$1.in
}

find $1 -type f -name "*.java" | sed "s|$1||" | while read file; do
  cp_java $file
done

rsync -a --delete $TMP_JAVA_DIR/ $TARGET_JAVA_DIR

declare -A VARIABLES
while  read var value ; do
  if [[ $var =~ [a-zA-Z] ]]; then
    VARIABLES[$var]=$value
  fi
done < <(cat $ROOT_DIR/version.txt)

APP_NAME_LC=${VARIABLES[APP_NAME],,}
#echo $APP_NAME_LC

find $TARGET_JAVA_DIR -name "*.java.in" |xargs -i sed -i -e "s/${VARIABLES[APP_PACKAGE]}/@APP_PACKAGE@/g" -e "s/${VARIABLES[APP_NAME]}/@APP_NAME@/g" -e "s/${APP_NAME_LC}/@APP_NAME_LC@/g" {}

rm -rf $TMP_JAVA_DIR
