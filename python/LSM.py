import math

# from collections import Counter
# from types import new_class
import numpy as np
from scipy.ndimage import median_filter
from scipy.stats import norm

# import scipy as sc
import cv2 as cv
from PIL import Image

def main():
    detection("../ir_images/jan17_test.jpg")


def detection(input_img):
    # filtering size for whitetophat
    filtering_size = (7, 7)

    # whitetophat filtering
    kernel = cv.getStructuringElement(cv.MORPH_RECT, filtering_size)
    img = cv.imread(input_img)
    img = cv.cvtColor(img, cv.COLOR_BGR2GRAY)
    whitetophat_img = cv.morphologyEx(img, cv.MORPH_TOPHAT, kernel)

    # Computing mean and standard deviation of image after morphing
    avg = np.mean(whitetophat_img)
    std = np.std(whitetophat_img)

    P = 1e-4 # false alarm probability
    a = 3 # adjustive coefficient

    # Compute adaptive threshold
    threshold = avg - a * norm.ppf(P) * std

    # Create a binary mask for suspicious targets
    suspicious_target_mask = whitetophat_img > threshold

    coordinates = np.column_stack(np.where(suspicious_target_mask))

    # Size of region, should be no larger than 9 x 9
    pixel_region = (5, 5)

    pixel_side = pixel_region[0]
    k = pixel_side // 2
    imagelength_height, imagelength_width = whitetophat_img.shape

    # centers for surrounding 8 regions (V_0 ~ V_8)
    centers = [
        ( -pixel_side - 1 ,  -pixel_side - 1),
        ( -pixel_side - 1, 0),
        ( -pixel_side - 1, pixel_side + 1),
        ( 0, -pixel_side - 1),
        ( 0, pixel_side + 1), 
        ( pixel_side + 1, -pixel_side - 1),
        ( pixel_side + 1, 0),
        ( pixel_side + 1, pixel_side + 1)
    ]

    num_neighbors = len(centers)

    '''
      values for p_k (potential target) consisting of 
        1. coordinates (x,y)
        2. maximum grey val (L_pk)
        3. average of surrounding medians (M_pk)
        4. beleavabiltiy measurements (w_pk)
    '''
    pk_vals = []
    C_pk = []

    for y, x in coordinates : 
        # Get V_0 patch
        min_x = max(x - k, 0)
        max_x = min(x + k + 1, imagelength_width)
        min_y = max(y - k, 0)
        max_y = min(y + k + 1, imagelength_height)

        patch = whitetophat_img[
            min_y : max_y, 
            min_x : max_x
        ]

        if len(patch) == 0:
            continue

        # Compute max grey value in patch
        L_pk = np.max(patch)

        # Get medians for surrounding V_1 ~ V_8
        medians = np.zeros(num_neighbors)
        # Get SSDs for surrounding V_1 ~ V_8
        SSDs = np.zeros(num_neighbors)

        # For each V_n
        for i, (new_x, new_y) in enumerate(centers):
            new_x = x + new_x
            new_y = y + new_y
            new_min_x = max(new_x - k, 0)
            new_max_x = min(new_x + k + 1, imagelength_width)
            new_min_y = max(new_y - k, 0)
            new_max_y = min(new_y + k + 1, imagelength_height)

            # V_n
            new_patch = whitetophat_img[
                new_min_y : new_max_y,
                new_min_x : new_max_x
            ]



            # median and SSD of V_n
            median = np.median(new_patch)
            SSD = np.sum((patch - new_patch) ** 2)

            medians[i] = median
            SSDs[i] = SSD

        # average of medians
        M_pk = np.average(medians)
        
        # average of SSDs
        w_pk = np.average(SSDs)

        pk_vals.append([(x, y), L_pk, M_pk, w_pk])
    

    # Assign W_pk = (w_pk - w_min) / (w_max, w_min)
    w_pk_s = [ row[-1] for row in pk_vals ]

    # Compute min of w_pk and max of w_pk
    min_w_pk = np.min(w_pk_s)
    max_w_pk = np.max(w_pk_s)

    # Local salient map
    LSM = []

    # Local contrast value calculation
    for i, pk_val in enumerate(pk_vals) :
        w_pk = pk_val[-1]
        W_pk = (w_pk - min_w_pk) / (max_w_pk - min_w_pk)

        L_pk = pk_val[1]
        M_pk = pk_val[2]
        
        C_pk = W_pk * (L_pk ** 2) / M_pk

        LSM.append([pk_val[0], C_pk])

    # Get MAX(LSM)
    max_lsm = float('-inf')
    for coordinates, lsm in LSM :
        max_lsm = max(max_lsm, lsm)

    # Compute threshold
    g = 0.5
    T = g * max_lsm

    # Detections
    result = []
    for coordinates, lsm in LSM :
        if lsm > T :
            result.append(coordinates)
    

    draw_bounding_boxes(result,img)
    return result


def draw_bounding_boxes(coordinates, img):
    # Define bounding box size
    box_size = 10  # Half-width/height of the bounding box

    # Draw bounding boxes around the detected coordinates
    for (x, y) in coordinates:
        # Calculate bounding box corners, ensuring they stay within image bounds
        top_left = (max(0, x - box_size), max(0, y - box_size))
        bottom_right = (min(img.shape[1] - 1, x + box_size), min(img.shape[0] - 1, y + box_size))

        # Draw the bounding box
        cv.rectangle(img, top_left, bottom_right, (255, 255, 255), 2)  # Green box, 2 px thickness

    # Display the image with bounding boxes
    cv.imshow("Detected Targets2", img)
    cv.waitKey(0)
    cv.destroyAllWindows()

    # Optionally save the image
    cv.imwrite("jan17_test_with_bounding_boxes.jpg", img) 

if __name__ == "__main__":
    main()
