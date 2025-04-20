import json
import pandas as pd
import numpy as np

def load_log(file_path):
    """
    Loads the JSON lines from the given file and returns a Pandas DataFrame
    with columns: Image_ID, Latitude, Longitude, Altitude, Roll, Pitch, Yaw, Timestamp.
    """
    data_list = []
    with open(file_path, "r") as file:
        for line in file:
            json_obj = json.loads(line.strip())
            data = json_obj["data"]
            
            data_list.append({
                "Image_ID": data.get("Img"),
                "Latitude": data.get("Lat"),
                "Longitude": data.get("Lng"),
                "Altitude": data.get("Alt"),
                "Roll": data.get("R"),    # degrees (assumed)
                "Pitch": data.get("P"),  # degrees (assumed)
                "Yaw": data.get("Y"),    # degrees (assumed)
                "Timestamp": data.get("TimeUS")
            })
    return pd.DataFrame(data_list)

def compute_3D_from_pixel(lat, lng, alt, roll_deg, pitch_deg, yaw_deg,
                         pixel_x, pixel_y, focal_length):
    # Convert angles to radians (assuming its in degrees)
    roll = np.radians(roll_deg)
    pitch = np.radians(pitch_deg)
    yaw = np.radians(yaw_deg)

    # Intrinsic Camera Matrix K
    K = np.array([
        [focal_length,        0,   0],
        [       0,     focal_length, 0],
        [       0,             0,   1]
    ], dtype=float)

    # Rotation matrices Rx, Ry, Rz
    Rx = np.array([
        [1,           0,           0],
        [0,  np.cos(roll), -np.sin(roll)],
        [0,  np.sin(roll),  np.cos(roll)]
    ], dtype=float)
    
    Ry = np.array([
        [ np.cos(pitch), 0, np.sin(pitch)],
        [             0, 1,             0],
        [-np.sin(pitch), 0, np.cos(pitch)]
    ], dtype=float)
    
    Rz = np.array([
        [ np.cos(yaw), -np.sin(yaw), 0],
        [ np.sin(yaw),  np.cos(yaw), 0],
        [          0,           0,   1]
    ], dtype=float)
    
    R_cam2world = Rz @ Ry @ Rx

    pixel_hom = np.array([pixel_x, pixel_y, 1], dtype=float)

    K_inv = np.linalg.inv(K)
    dir_cam = K_inv @ pixel_hom  # direction in camera coords

    dir_world = R_cam2world @ dir_cam

    if abs(dir_world[2]) < 1e-12:
        return np.array([np.nan, np.nan, np.nan])

    scale = alt / dir_world[2]

    camera_center = np.array([lat, lng, alt], dtype=float)

    point_world = camera_center + scale * dir_world

    return point_world

# ------------------------------------------------------------------------
# Example usage:
df = load_log("parse_text/trig.txt")
print("Parsed DataFrame:\n", df, "\n")

focal_length = 5000  # example focal length in pixels from geogebra

example = df.iloc[0]
print("Example row:\n", example, "\n")

# Suppose we want to see where the ray from pixel = (2736, 1824) ends up,
# forcing Z = Altitude in our simplistic approach:
xyz_world = compute_3D_from_pixel(
    lat=example["Latitude"],
    lng=example["Longitude"],
    alt=example["Altitude"],
    roll_deg=example["Roll"],
    pitch_deg=example["Pitch"],
    yaw_deg=example["Yaw"],
    pixel_x=2736,
    pixel_y=1824,
    focal_length=focal_length
)

print("Computed 3D world point from pixel (2736, 1824):", xyz_world)
