#!/bin/bash
# SPDX-FileCopyrightText: 2025 Tobias Fella <tobias.fella@kde.org>
# SPDX-License-Identifier: GPL-2.0-or-later

file="$1/QuotientQt6plugin.qmltypes"

if [ ! -f "$file" ]; then
    echo "File $file does not exist"
    exit 1
fi

lines=$(wc -l < "$file")
if [ "$lines" -gt 100 ]; then
    exit 0
else
    exit 1
fi

