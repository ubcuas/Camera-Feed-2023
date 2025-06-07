import json


def check_monotonic_timestamps(file_path):
    prev_timestamp = None
    line_number = 0

    with open(file_path, 'r') as file:
        for line in file:
            line_number += 1
            try:
                record = json.loads(line)
                timestamp = record["meta"]["timestamp"]

                if prev_timestamp is not None and timestamp < prev_timestamp:
                    print(f"Timestamps not monotonic at line {line_number}: {timestamp} < {prev_timestamp}")
                    return False

                prev_timestamp = timestamp

            except (json.JSONDecodeError, KeyError) as e:
                print(f"Error parsing line {line_number}: {e}")
                return False

    print("Timestamps are monotonically increasing.")
    return True

check_monotonic_timestamps("full.txt")