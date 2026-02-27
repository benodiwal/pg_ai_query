#!/bin/bash

REAL_USER=${SUDO_USER:-$USER}
USER_HOME=$(getent passwd "$REAL_USER" | cut -d: -f6)

if [ -z "$USER_HOME" ]; then
    echo "Error: Could not determine home directory for $REAL_USER."
    exit 1
fi

SOURCE_FILE=".psqlrc_pg_ai_query"
TARGET_EXT_FILE="$USER_HOME/.psqlrc_pg_ai_query"
MAIN_PSQLRC="$USER_HOME/.psqlrc"
INCLUDE_LINE="\\i '$TARGET_EXT_FILE'"

if [ -f "$SOURCE_FILE" ]; then
    cp "$SOURCE_FILE" "$TARGET_EXT_FILE"

    chown "$REAL_USER" "$TARGET_EXT_FILE"
    chmod 600 "$TARGET_EXT_FILE"
else
    echo "Error: Source file $SOURCE_FILE not found in the current directory."
    exit 1
fi

if [ ! -f "$MAIN_PSQLRC" ]; then
    echo "  Creating new $MAIN_PSQLRC and adding include line."
    echo -e "-- PostgreSQL Configuration\n$INCLUDE_LINE" > "$MAIN_PSQLRC"
    chown "$REAL_USER" "$MAIN_PSQLRC"
    chmod 600 "$MAIN_PSQLRC"
elif grep -Fq "$TARGET_EXT_FILE" "$MAIN_PSQLRC"; then
    echo "  Include line already exists in $MAIN_PSQLRC. Skipping."
else
    echo "  Appending include line to $MAIN_PSQLRC"
    echo -e "\n$INCLUDE_LINE" >> "$MAIN_PSQLRC"
fi

echo "Successfully configured .psqlrc for $REAL_USER."