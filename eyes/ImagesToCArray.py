import os
import sys
import cv2
import numpy as np
import random

def image_to_c_array(prev_img, curr_img, image_name):
    height, width, _ = curr_img.shape
    
    white_pixels = []
    black_pixels = []
    
    for y in range(height):
        for x in range(width):
            prev_pixel = prev_img[y, x] if prev_img is not None else np.array([0, 0, 0])
            curr_pixel = curr_img[y, x]
            
            prev_is_colored = any(prev_pixel > 128)
            curr_is_colored = any(curr_pixel > 128)
            
            if prev_is_colored != curr_is_colored:  # Change detected
                if curr_is_colored:
                    white_pixels.append((x, y))
                else:
                    black_pixels.append((x, y))
    random.shuffle(white_pixels)
    random.shuffle(black_pixels)
    white_pixels = [(x, y) for x, y in white_pixels if x < 80]
    black_pixels = [(x, y) for x, y in black_pixels if x < 80]
    # Convert to C array format
    white_array = f"const unsigned char white_pixels_{image_name}[][2] PROGMEM = {{" + ", ".join(f"{{{x},{y}}}" for x, y in white_pixels) + "};"
    black_array = f"const unsigned char black_pixels_{image_name}[][2] PROGMEM= {{" + ", ".join(f"{{{x},{y}}}" for x, y in black_pixels) + "};"
    
    return white_array, black_array

def process_images_in_directory(directory):
    filenames = sorted([f for f in os.listdir(directory) if f.lower().endswith(('png', 'jpg', 'jpeg', 'bmp'))])
    prev_img = None
    
    for filename in filenames:
        image_path = os.path.join(directory, filename)
        image_name = os.path.splitext(filename)[0]
        curr_img = cv2.imread(image_path)
        
        if curr_img is None:
            print(f"Error loading image: {image_path}")
            continue
        
        white_array, black_array = image_to_c_array(prev_img, curr_img, image_name)
        print(f"// {filename}")
        print(white_array)
        print(black_array)
        print()
        
        prev_img = curr_img.copy()

# Example usage
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python ImagesToCArray.py <image_directory>")
        sys.exit(1)
    image_directory = sys.argv[1]
    process_images_in_directory(image_directory)
