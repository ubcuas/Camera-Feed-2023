from datetime import datetime, timezone

import numpy as np


class MockCameraController:
    def __init__(self):
        pass

    def setup(self, node_keyval):
        for dict in node_keyval:
            k, v = next(iter(dict.items()))
            print(f'{k}: {v}')

    def setup_tl(self, tl_stream_nodes_key_val):
        pass

    def _set_time(self):
        pass

    def _create_devices_with_tries(self):
        pass

    def start_stream(self):
        pass

    def stop_stream(self):
        pass

    def get_npimage(self):
        timestamp = datetime.now(timezone.utc)
        with open('./tests/data/test2.npy', 'rb') as f:
            npndarray = np.load(f)
        return npndarray, timestamp

    def _reset_settings(self):
        pass

    def cleanup(self):
        pass