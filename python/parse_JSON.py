import argparse
import json

import cv2
import numpy as np
from typing import List, Dict, Tuple
from geographiclib.geodesic import Geodesic
import matplotlib.pyplot as plt
import plotly.express as px
import pandas as pd

# f_x, f_y = 2500, 2500       # focal length in pixels
# c_x, c_y = 0, 0             # optical center in pixels
# img_w, img_h = 2736, 1824   # image resolution
f_x, f_y = 2482.29888, 2485.09241       # focal length in pixels
c_x, c_y = 1358.69991, 904.861872           # optical center in pixels
img_w, img_h = 2736, 1824   # image resolution

center_x, center_y = img_w // 2, img_h // 2

# intrinsic camera matrix
intrinsic = np.array([
    [f_x,  0, c_x],
    [0,  f_y, c_y],
    [0,    0,   1]]
)
dist = np.array([[-5.28353529e-02, -9.38327045e-02, -1.27634646e-04, -2.01700049e-03, 4.64714290e-01]])
geod = Geodesic.WGS84

tol = 200000


# def load_trig_log(file_path: str) -> List[Dict]:
#     """
#     Loads the JSON lines from the given file and returns a Pandas DataFrame
#     with columns: Image_ID, Latitude, Longitude, Altitude, Roll, Pitch, Yaw, Timestamp.
#     """
#     trig_list = []
#     with open(file_path, "r") as file:
#         for line in file:
#             json_obj = json.loads(line.strip())
#             data = json_obj["data"]
#
#             trig_list.append(data)
#     return trig_list
#
#
# def load_detect_log(file_path: str) -> List[Dict]:
#     detect_list = []
#     with open(file_path, "r") as file:
#         for line in file:
#             json_obj = json.loads(line.strip())
#             detect_list.append(json_obj)
#     return detect_list

def load_log(file_path: str) -> List[Dict]:
    log = []
    with open(file_path, "r") as file:
        for line in file:
            json_obj = json.loads(line.strip())
            log.append(json_obj)
    return log

# def load_log(file_path: str) -> List[Dict]:
#     return [
#         {"TimeUS": 1746269284621072, "Img": 2, "Points": [[0, 0], [img_w, img_h], [img_w, 0], [0, img_h]], "Epoch": 1746268941974857, "Delta_t": -932,
#          "Feedback": {"time_usec": 342647147, "img_idx": 1393, "lat": 49.259629434902564, "lng": -123.24856966776673, "alt_msl": 0,
#                       "alt_rel": 100.0, "roll": 10.0, "pitch": 10.0,
#                       "yaw": 10.0, "completed_captures": 64}}
#
#     ]
# def load_trig_log(filename):
#     return [
#         # {"TimeUS": 1000, "I": 0, "Img": 1, "GPSTime": 0, "GPSWeek": 0, "Lat": 49.259629434902564, "Lng": -123.24856966776673, "Alt": 100.0, "RelAlt": 100.0, "GPSAlt": 100.0, "R": 10.0, "P": 10.0, "Y": 10.0},
#         {"TimeUS": 1000, "I": 0, "Img": 1, "GPSTime": 0, "GPSWeek": 0, "Lat": 49.259629434902564, "Lng": -123.24856966776673, "Alt": 100.0, "RelAlt": 100.0, "GPSAlt": 100.0, "R": -10.0, "P": -10.0, "Y": -10.0}
#
#         # {"Timestamp": 1100, "Latitude": 37.7750, "Longitude": -122.4195, "Alt": 60, "Roll": 0, "Pitch": 0, "Yaw": 45},
#         # {"Timestamp": 1200, "Latitude": 37.7751, "Longitude": -122.4196, "Alt": 70, "Roll": 2, "Pitch": 2, "Yaw": 2},
#         # {"Timestamp": 1300, "Latitude": 37.7752, "Longitude": -122.4197, "Alt": 80, "Roll": 3, "Pitch": 3, "Yaw": 3},
#         # {"Timestamp": 1400, "Latitude": 37.7753, "Longitude": -122.4198, "Alt": 90, "Roll": 4, "Pitch": 4, "Yaw": 4},
#     ]
#
#
# def load_detect_log(filename):
#     return [
#         # {"Img": 1, "TimeUS": 2000, "Points": [[0, 0], [img_w, img_h], [img_w, 0], [0, img_h]]},
#         {"Img": 1, "TimeUS": 2000, "Points": [[0, 0], [img_w, img_h], [img_w, 0], [0, img_h]]},
#
#         # {"Timestamp": 2100, "Points": [[0, 0], [img_w, img_h], [img_w, 0], [0, img_h]]},
#         # {"Timestamp": 2200, "Points": [[0, 0], [img_w, img_h], [img_w, 0], [0, img_h]]},
#         # {"Timestamp": 2300, "Points": [[0, 0], [img_w, img_h], [img_w, 0], [0, img_h]]},
#         # {"Timestamp": 2400, "Points": [[0, 0], [img_w, img_h], [img_w, 0], [0, img_h]]},
#     ]


def compute_extrinsic(roll_deg: float, pitch_deg: float, yaw_deg: float) -> np.ndarray:
    # Convert angles to radians (assuming its in degrees)
    roll = np.radians(roll_deg)
    pitch = np.radians(pitch_deg)
    yaw = np.radians(yaw_deg)

    # Rotation matrix about X-axis
    Rx = np.array([
        [1, 0, 0],
        [0, np.cos(pitch), -np.sin(pitch)],
        [0, np.sin(pitch), np.cos(pitch)]
    ], dtype=float)

    # Rotation matrix about Y-axis
    Ry = np.array([
        [np.cos(roll), 0, np.sin(roll)],
        [0, 1, 0],
        [-np.sin(roll), 0, np.cos(roll)]
    ], dtype=float)

    # Rotation matrix about Z-axis
    Rz = np.array([
        [np.cos(yaw), -np.sin(yaw), 0],
        [np.sin(yaw), np.cos(yaw), 0],
        [0, 0, 1]
    ], dtype=float)

    R = Rx @ Ry @ Rz

    return R


def compute_offset(alt: float, extrinsic: np.ndarray, pixel_x: int, pixel_y: int) -> Tuple[float, float]:
    # pixel_x = pixel_x - center_x
    # pixel_y = center_y - pixel_y
    pixel_y = img_h - pixel_y
    distorted = np.array([[[pixel_x, pixel_y]]], dtype=np.float32)
    undistorted = cv2.undistortPoints(distorted, intrinsic, dist, P=intrinsic)
    undistorted_pixel = undistorted[0][0]  # (x, y) in pixel coordinates

    pixel_hom = np.array([undistorted_pixel[0], undistorted_pixel[1], 1.0], dtype=float)
    world_vec = extrinsic.T @ np.linalg.inv(intrinsic) @ pixel_hom

    # if abs(dir_world[2]) < 1e-12:
    #     return np.array([np.nan, np.nan, np.nan])

    scale = alt / world_vec[2]

    offset = world_vec * scale
    x, y = offset[0], offset[1]
    return x, y


def compute_geoposition(lat: float, lng: float, x: float, y: float) -> Tuple[float, float]:
    result = geod.Direct(lat, lng, 90, x)
    new_lat, new_lng = result['lat2'], result['lon2']
    result = geod.Direct(new_lat, new_lng, 0, y)
    new_lat, new_lng = result['lat2'], result['lon2']

    return new_lat, new_lng


def plot_gps(coords, ground_truth=None):
    color_scale = px.colors.sequential.Viridis  # or use 'Turbo', 'Plasma', etc.

    fig = px.scatter_map(
        coords,
        lat="Lat",
        lon="Lng",
        hover_data=["ID", "Lat", "Lng", "RelAlt"],
        color="RelAlt",
        color_continuous_scale=color_scale,  # Gradient color
        zoom=14
    )

    # If ground truth coordinates are provided, plot them on top with a different color or style
    if ground_truth is not None:
        fig.add_scattermap(
            lat=ground_truth["Lat"],
            lon=ground_truth["Lng"],
            mode="markers",
            marker=dict(color="red"),  # Customize marker size and color
            name="Ground Truth"
        )

    fig.update_layout(map_style="dark")
    fig.update_layout(margin={"r": 0, "t": 0, "l": 0, "b": 0})
    fig.show()


# def match_coords(predictions, ground_truth):
#     results = []
#
#     for coord_p in predictions:
#         min_distance = float('inf')
#         closest = None
#         for coord_g in ground_truth:
#             g = geod.Inverse(coord_p[0], coord_p[1], coord_g[0], coord_g[1])
#             dist = g['s12']  # Distance in meters
#             if dist < min_distance:
#                 min_distance = dist
#                 closest = coord_b
#         results.append({
#             'from': coord_a,
#             'to': closest_b,
#             'distance_km': min_distance
#         })
#
#     return results


# def coords_to_placemark_kml(coords, filename="output.kml", names=None, description=""):
#     with open(filename, 'w') as f:
#         f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
#         f.write('<kml xmlns="http://www.opengis.net/kml/2.2">\n')
#         f.write(' <Document>\n')
#
#         for i, (lat, lon) in enumerate(coords):
#             name = names[i] if names and i < len(names) else f"Hotspot {i}"
#             desc = description if i == 0 else ""
#             f.write('  <Placemark>\n')
#             f.write(f'   <name>{name}</name>\n')
#             if desc:
#                 f.write(f'   <description>{desc}</description>\n')
#             f.write('   <Point>\n')
#             f.write(f'    <coordinates>{lon},{lat},0</coordinates>\n')
#             f.write('   </Point>\n')
#             f.write('  </Placemark>\n')
#
#         f.write(' </Document>\n')
#         f.write('</kml>\n')
# def coords_to_placemark_kml(coords, filename="output.kml"):
#     with open(filename, 'w') as f:
#         f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
#         f.write('<kml xmlns="http://www.opengis.net/kml/2.2">\n')
#         f.write('   <Document>\n')
#
#         for i, (lat, lon) in enumerate(coords):
#             f.write('       <Placemark>\n')
#             f.write('           <Point>\n')
#             f.write(f'              <coordinates>{lon},{lat},0</coordinates>\n')
#             f.write('           </Point>\n')
#             f.write('       </Placemark>\n')
#
#         f.write('   </Document>\n')
#         f.write('</kml>\n')

def main():
    parser = argparse.ArgumentParser(description="Process log files.")
    parser.add_argument('-l', '--log_path', type=str, help='Path to log file')

    args = parser.parse_args()

    logs = load_log(args.log_path)
    coords = []

    for l in logs:
        drone_lat = l.get("Feedback").get("lat") / 1e7
        drone_lng = l.get("Feedback").get("lng") / 1e7
        alt = l.get("Feedback").get("alt_rel")
        roll = l.get("Feedback").get("roll")
        pitch = l.get("Feedback").get("pitch")
        yaw = l.get("Feedback").get("yaw")
        points = l.get("Points")
        id = l.get("Img")
        for p in points:
            # TODO add limiter
            detect_lat, detect_lng = cam2Geoposition(pitch, roll, yaw, alt, p[0], p[1], drone_lat, drone_lng)

            # Store the values for plotting
            # TODO: get id for image and log
            coords.append([id, detect_lat, detect_lng, alt])

    coords = pd.DataFrame(coords, columns=["ID", "Lat", "Lng", "RelAlt"])
    # ground_truth = pd.DataFrame([[49.259629434902564, -123.24856966776673]], columns=["Lat", "Lng"])
    # plot_gps(coords, ground_truth)
    coords.to_csv('detections.csv')
    print("Saved to detections.csv")
    plot_gps(coords)


def cam2Geoposition(pitch, roll, yaw, alt, pixel_x, pixel_y, drone_lat, drone_lng):
    extrinsic = compute_extrinsic(roll, pitch, yaw)
    x, y = compute_offset(alt, extrinsic, pixel_x, pixel_y)
    detect_lat, detect_lng = compute_geoposition(drone_lat, drone_lng, x, y)
    return detect_lat, detect_lng


if __name__ == '__main__':
    main()
# ------------------------------------------------------------------------
# # Example usage:
# df = load_log("parse_text/trig.txt")
# # print("Parsed DataFrame:\n", df, "\n")
#
# example = df.iloc[0]
# print("Example row:\n", example, "\n")

# Suppose we want to see where the ray from pixel = (2736, 1824) ends up,
# forcing Z = Altitude in our simplistic approach:
# xyz_world = compute_3D_from_pixel(
#     lat=example["Latitude"],
#     lng=example["Longitude"],
#     alt=example["Altitude"],
#     roll_deg=example["Roll"],
#     pitch_deg=example["Pitch"],
#     yaw_deg=example["Yaw"],
#     pixel_x=2736,
#     pixel_y=1824,
#     focal_length=focal_length
# )

# print("Computed 3D world point from pixel (2736, 1824):", xyz_world)
