#!/bin/bash
# scp uascv@uas.local:~/Camera-Feed-2023/tag.txt .
source venv/Scripts/activate && python python/parse_JSON.py -l tag.txt
python python/generate_kml.py hotspots.csv