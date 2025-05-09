import argparse
import heapq
import json

import cv2
import numpy as np
from typing import List, Dict, Tuple
import plotly.express as px
import pandas as pd
from sklearn.cluster import DBSCAN
from geopy.distance import great_circle
from shapely.geometry import MultiPoint
import random
from geographiclib.geodesic import Geodesic


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


def load_log(file_path: str) -> List[Dict]:
    log = []
    with open(file_path, "r") as file:
        i = 1
        for line in file:
            try:
                json_obj = json.loads(line.strip())
                if isinstance(json_obj.get("Points"), list) and len(json_obj["Points"]) > 0:
                    log.append(json_obj)
            except json.JSONDecodeError as e:
                print(f"JSONDecodeError on line {i}: {e}")
                print(f"Content: {line.strip()}")

            i += 1
    return log

# def load_log(file_path: str) -> List[Dict]:
#     return [
#         {"TimeUS": 1746269284621072, "Img": 2, "Points": [[0, 0], [img_w, img_h], [img_w, 0], [0, img_h]], "Epoch": 1746268941974857, "Delta_t": -932,
#          "Feedback": {"time_usec": 342647147, "img_idx": 1393, "lat": 492596294, "lng": -1232485696, "alt_msl": 0,
#                       "alt_rel": 100.0, "roll": 0, "pitch": 0,
#                       "yaw": 0, "completed_captures": 64}}
#
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


def plot_gps(coords, centroids=None):
    """
    Plot GPS coordinates on a map, optionally with cluster centroids.

    Args:
        coords: DataFrame with 'Lat' and 'Lng' columns for all points
        centroids: DataFrame with 'Lat' and 'Lng' columns for cluster centroids (optional)
    """
    # Create figure with original points
    fig = px.scatter_map(
        coords,
        lat="Lat",
        lon="Lng",
        zoom=16,
        color_discrete_sequence=['blue'],
        opacity=0.7,
        size_max=10,
        map_style="dark",
    )

    # Add centroids if provided
    if centroids is not None and not centroids.empty:
        centroid_trace = px.scatter_map(
            centroids,
            lat="Lat",
            lon="Lng",
            color_discrete_sequence=['red'],
            size_max=15,
        ).data[0]

        # Update marker size and symbol for centroids
        centroid_trace.marker = dict(size=15, symbol="circle", color="red")
        fig.add_trace(centroid_trace)

    # Update layout
    fig.update_layout(
        margin={"r": 0, "t": 0, "l": 0, "b": 0},
        legend=dict(
            orientation="h",
            yanchor="bottom",
            y=1.02,
            xanchor="right",
            x=1
        )
    )

    fig.show()


def cluster(coords):
    """
    Cluster coordinates using DBSCAN algorithm with haversine distance.

    Args:
        coords: DataFrame with 'Lat' and 'Lng' columns

    Returns:
        DataFrame with representative points for each cluster
    """
    # Convert to numpy array for DBSCAN
    X = coords[["Lat", "Lng"]].values

    # Define parameters for DBSCAN
    meter_per_radian = 6371000.0  # Earth radius in meters
    epsilon = 12 / meter_per_radian  # 12 meters
    min_samples = 3

    # Apply DBSCAN
    db = DBSCAN(eps=epsilon, min_samples=min_samples, algorithm='ball_tree', metric='haversine').fit(np.radians(X))
    cluster_labels = db.labels_

    # Filter out noise points (label -1)
    unique_labels = [label for label in set(cluster_labels) if label != -1]
    num_clusters = len(unique_labels)
    print('Number of clusters: {} (Choose 8 largest)'.format(num_clusters))

    # Create list to store centermost points
    centermost_points = []
    clusters = []
    # Process each cluster
    for label in unique_labels:
        # Get points in the cluster
        cluster_points = coords[cluster_labels == label]

        # Extract coordinates as list of (lat, lng) tuples for MultiPoint
        points_list = [(point.Lat, point.Lng) for _, point in cluster_points.iterrows()]
        clusters.append(points_list)
    largest_clusters = heapq.nlargest(8, clusters, key=len)

    for points_list in largest_clusters:
        # Create MultiPoint object
        if points_list:
            multi_point = MultiPoint(points_list)
            centroid = (multi_point.centroid.x, multi_point.centroid.y)

            centermost_points.append(centroid)

    # If we found any clusters, create a DataFrame with the centermost points
    if centermost_points:
        lats, lngs = zip(*centermost_points)
        rep_points = pd.DataFrame({'Lat': lats, 'Lng': lngs})
        return rep_points
    else:
        # Return empty DataFrame if no clusters found
        return pd.DataFrame(columns=['Lat', 'Lng'])


def generate_gps_cluster(center_lat, center_lon, num_points, radius_m=12):
    cluster = []

    for _ in range(num_points):
        # Random bearing [0, 360) and distance [0, radius_m)
        bearing = random.uniform(0, 360)
        distance = random.uniform(0, radius_m)

        # Compute destination point
        destination = geod.Direct(center_lat, center_lon, bearing, distance)
        cluster.append((destination['lat2'], destination['lon2']))

    return cluster


def get_centermost_point(cluster):
    centroid = (MultiPoint(cluster).centroid.x, MultiPoint(cluster).centroid.y)
    centermost_point = min(cluster, key=lambda point: great_circle(point, centroid).m)
    return tuple(centermost_point)



def main():
    parser = argparse.ArgumentParser(description="Process log files.")
    parser.add_argument('-l', '--log_path', type=str, required=True, help='Path to log file')
    # parser.add_argument('-d', '--detect_path', type=str, help='Path to detect file')


    args = parser.parse_args()
    if args.log_path:
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
            for p in points:
                detect_lat, detect_lng = cam2Geoposition(pitch, roll, yaw, alt, p[0], p[1], drone_lat, drone_lng)
                coords.append([detect_lat, detect_lng])
        coords = pd.DataFrame(coords, columns=["Lat", "Lng"])
        centroids = cluster(coords)
        plot_gps(coords, centroids)
        centroids.to_csv("hotspots.csv", index=False)
        print("Coords written to hotspots.csv")
    #
    # coords = generate_gps_cluster(49.2596294, -123.2485696, 5) + \
    #          generate_gps_cluster(49.2599294, -123.2485696, 7) + \
    #          generate_gps_cluster(49.2596294, -123.2489696, 3) + \
    #          generate_gps_cluster(49.2599294, -123.2489696, 5) + \
    #          generate_gps_cluster(49.2588294, -123.2469696, 12) + \
    #          generate_gps_cluster(49.2588294, -123.2478696, 5) + \
    #          generate_gps_cluster(49.2583294, -123.2470696, 5) + \
    #          generate_gps_cluster(49.2583294, -123.2480696, 8) + \
    #          generate_gps_cluster(49.2588294, -123.2460696, 4)
    #
    # coords = pd.DataFrame(coords, columns=["Lat", "Lng"])
    # centroids = cluster(coords)
    # plot_gps(coords, centroids)
    # centroids.to_csv("hotspots.csv", index=False)


    # plotFrame(0, 0, 0, 100, 492596294/1e7, -1232485696/1e7, [[0, 0], [img_w, img_h], [img_w, 0], [0, img_h]])


def plotFrame(pitch, roll, yaw, alt, drone_lat, drone_lng, points):
    coords = []

    for p in points:
        detect_lat, detect_lng = cam2Geoposition(pitch, roll, yaw, alt, p[0], p[1], drone_lat, drone_lng)
        coords.append([detect_lat, detect_lng])
    coords = pd.DataFrame(coords, columns=["Lat", "Lng"])
    center = pd.DataFrame([[drone_lat, drone_lng]], columns=["Lat", "Lng"])
    plot_gps(coords, center)

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
