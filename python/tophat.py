import argparse
import os
import json
import cv2
from PIL import Image, ImageDraw


def process_image(image_path):
    image = cv2.imread(image_path, cv2.IMREAD_GRAYSCALE)
    Kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (21, 21))
    whitehat = cv2.morphologyEx(image, cv2.MORPH_TOPHAT, Kernel)

    _, th2 = cv2.threshold(whitehat, 100, 255, cv2.THRESH_BINARY)
    num_labels, labels, stats, centroids = cv2.connectedComponentsWithStats(th2)

    tophat = Image.fromarray(whitehat)
    draw = ImageDraw.Draw(tophat)

    # Draw red circles on the centroids
    for i in range(1, num_labels):  # Start from 1 to skip the background
        # Get the centroid for each component
        cx, cy = centroids[i]

        # Draw a red circle at the centroid
        radius = 20  # Circle radius
        draw.ellipse([cx - radius, cy - radius, cx + radius, cy + radius], outline="white", width=2)
    tophat.show()

    timestamp = os.path.splitext(os.path.basename(image_path))[0]
    return {"TimeUS": int(timestamp), "Points": centroids[1:].tolist()}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-i", "--input", required=True, help="Path to an image or a directory of images")
    ap.add_argument("-o", "--output", required=True, help="Path to output text file")
    args = ap.parse_args()

    entries = []

    if os.path.isdir(args.input):
        image_files = sorted([
            os.path.join(args.input, f) for f in os.listdir(args.input)
            if f.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp', '.tif', '.tiff'))
        ])
    else:
        image_files = [args.input]

    for image_path in image_files:
        try:
            entry = process_image(image_path)
            entries.append(entry)
        except Exception as e:
            print(f"Error processing {image_path}: {e}")

    with open(args.output, 'w') as out_file:
        for entry in entries:
            out_file.write(json.dumps(entry) + "\n")


if __name__ == "__main__":
    main()
#
#
# import argparse
# import cv2
# from PIL import Image, ImageDraw
#
# ap = argparse.ArgumentParser()
# ap.add_argument("-i", "--image", required=True, help="Path to the image")
# args = vars(ap.parse_args())
#
# image = cv2.imread(args["image"], cv2.IMREAD_GRAYSCALE)
# Kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (13, 13))
# whitehat = cv2.morphologyEx(image, cv2.MORPH_TOPHAT, Kernel)
#
# ret2,th2 = cv2.threshold(whitehat,100,255,cv2.THRESH_BINARY)
# cmap = cv2.applyColorMap(image, cv2.COLORMAP_JET)
# num_labels, labels, stats, centroids = cv2.connectedComponentsWithStats(th2)
#
# plt_image = cv2.cvtColor(cmap, cv2.COLOR_RGB2BGR)
# tophat = Image.fromarray(plt_image)
# draw = ImageDraw.Draw(tophat)
#
# # Draw red circles on the centroids
# for i in range(1, num_labels):  # Start from 1 to skip the background
#     # Get the centroid for each component
#     cx, cy = centroids[i]
#
#     # Draw a red circle at the centroid
#     radius = 20  # Circle radius
#     draw.ellipse([cx - radius, cy - radius, cx + radius, cy + radius], outline="white", width=2)
# tophat.show()


# x = np.arange(gray.shape[1])
# y = np.arange(gray.shape[0])
#
#
# X, Y = np.meshgrid(x, y)
#
# Z = gray
#
# fig = plt.figure(figsize=(15,15))
# ax1 = fig.add_subplot(111, projection='3d')
#
# ax1.set_title('viridis color map')
# surf1 = ax1.plot_surface(X, Y, Z)
# fig.colorbar(surf1, ax=ax1, shrink=0.5, aspect=5)
#
# plt.show()