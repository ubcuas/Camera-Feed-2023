import numpy as np
import cv2
import glob

chessboard_size = (9, 6)  # 9x6 chessboard
square_size = 1.0         
objp = np.zeros((chessboard_size[0]*chessboard_size[1], 3), np.float32)
objp[:, :2] = np.mgrid[0:chessboard_size[0], 0:chessboard_size[1]].T.reshape(-1, 2)
objp *= square_size

objpoints = []  
imgpoints = []  

images = glob.glob('calibration_images/*.jpg')  # put your images here

for fname in images:
    img = cv2.imread(fname)
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    ret, corners = cv2.findChessboardCorners(gray, chessboard_size, None)

    if ret:
        objpoints.append(objp)
        corners2 = cv2.cornerSubPix(gray, corners, (11,11), (-1,-1),
                                    criteria=(cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001))
        imgpoints.append(corners2)

        cv2.drawChessboardCorners(img, chessboard_size, corners2, ret)
        cv2.imshow('Corners', img)
        cv2.waitKey(200)

cv2.destroyAllWindows()

ret, K, dist, rvecs, tvecs = cv2.calibrateCamera(objpoints, imgpoints, gray.shape[::-1], None, None)

print("Camera matrix (K):\n", K)
print("Distortion coefficients:\n", dist)

np.savez("calibration_data.npz", K=K, dist=dist, rvecs=rvecs, tvecs=tvecs)
