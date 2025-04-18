import argparse
import cv2
from PIL import Image, ImageDraw


# construct the argument parser and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-i", "--image", required=True, help="Path to the image")
args = vars(ap.parse_args())

image = cv2.imread(args["image"], cv2.IMREAD_GRAYSCALE)
Kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (13, 13))
whitehat = cv2.morphologyEx(image, cv2.MORPH_TOPHAT, Kernel)

ret2,th2 = cv2.threshold(whitehat,100,255,cv2.THRESH_BINARY)
cmap = cv2.applyColorMap(image, cv2.COLORMAP_JET)
num_labels, labels, stats, centroids = cv2.connectedComponentsWithStats(th2)

plt_image = cv2.cvtColor(cmap, cv2.COLOR_RGB2BGR)
tophat = Image.fromarray(plt_image)
draw = ImageDraw.Draw(tophat)

# Draw red circles on the centroids
for i in range(1, num_labels):  # Start from 1 to skip the background
    # Get the centroid for each component
    cx, cy = centroids[i]

    # Draw a red circle at the centroid
    radius = 20  # Circle radius
    draw.ellipse([cx - radius, cy - radius, cx + radius, cy + radius], outline="white", width=2)
tophat.show()

#
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