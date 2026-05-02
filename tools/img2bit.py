#!/usr/bin/env python3
import argparse
import sys
import os

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow library is not installed. Please install it using 'pip install Pillow'")
    sys.exit(1)

def quantize_value(val, bpp, threshold):
    if bpp == 1:
        return 1 if val >= threshold else 0
    elif bpp == 2:
        return val // 64  # 0 to 3
    elif bpp == 4:
        return val // 16  # 0 to 15
    return val

def rgb_to_rgb332(r, g, b):
    r_3 = (r >> 5) & 0x07
    g_3 = (g >> 5) & 0x07
    b_2 = (b >> 6) & 0x03
    return (r_3 << 5) | (g_3 << 2) | b_2

def invert_value(val, bpp):
    if bpp == 1:
        return 1 - val
    elif bpp == 2:
        return 3 - val
    elif bpp == 4:
        return 15 - val
    elif bpp == 8:
        return 255 - val
    return val

def process_image(args):
    try:
        img = Image.open(args.input)
    except Exception as e:
        print(f"Error opening image: {e}")
        sys.exit(1)

    if args.width or args.height:
        new_w = args.width if args.width else img.width
        new_h = args.height if args.height else img.height
        img = img.resize((new_w, new_h), Image.Resampling.LANCZOS)
    
    pixels = []
    
    if args.bpp == 8:
        img = img.convert("RGB")
        for y in range(img.height):
            row = []
            for x in range(img.width):
                r, g, b = img.getpixel((x, y))
                val = rgb_to_rgb332(r, g, b)
                if args.invert:
                    val = invert_value(val, args.bpp)
                row.append(val)
            pixels.append(row)
    else:
        img = img.convert("L")
        for y in range(img.height):
            row = []
            for x in range(img.width):
                val = img.getpixel((x, y))
                val = quantize_value(val, args.bpp, args.threshold)
                if args.invert:
                    val = invert_value(val, args.bpp)
                row.append(val)
            pixels.append(row)
            
    packed_data = []
    
    if args.bpp == 8:
        for row in pixels:
            packed_data.extend(row)
            if args.stride and len(row) < args.stride:
                packed_data.extend([0] * (args.stride - len(row)))
    else:
        pixels_per_byte = 8 // args.bpp
        shift_amount = args.bpp
        
        for row in pixels:
            current_byte = 0
            bits_packed = 0
            row_bytes = 0
            
            for pixel_val in row:
                # MSB first packing
                shift = 8 - bits_packed - shift_amount
                current_byte |= (pixel_val << shift)
                bits_packed += shift_amount
                
                if bits_packed == 8:
                    packed_data.append(current_byte)
                    row_bytes += 1
                    current_byte = 0
                    bits_packed = 0
            
            # Pad the end of the row if not byte-aligned
            if bits_packed > 0:
                packed_data.append(current_byte)
                row_bytes += 1
                
            # Pad row to match explicit stride if provided
            if args.stride:
                if row_bytes < args.stride:
                    packed_data.extend([0] * (args.stride - row_bytes))
                elif row_bytes > args.stride:
                    print(f"Warning: Calculated row bytes ({row_bytes}) exceeds specified stride ({args.stride}). Image may be corrupted.")
                
    return packed_data

def output_data(data, args):
    if args.format == 'bin':
        if not args.output:
            print("Error: Binary format requires an output file (-o).")
            sys.exit(1)
        with open(args.output, 'wb') as f:
            f.write(bytearray(data))
        print(f"Wrote {len(data)} bytes to {args.output}")
        
    elif args.format == 'asm':
        out = sys.stdout if not args.output else open(args.output, 'w')
        out.write(f"{args.label}:\n")
        
        for i in range(0, len(data), 8):
            chunk = data[i:i+8]
            hex_strs = [f"0x{b:02X}" for b in chunk]
            out.write(f".db {', '.join(hex_strs)}\n")
            
        if args.output:
            out.close()
            print(f"Wrote assembly to {args.output}")
            
    elif args.format == 'hex':
        out = sys.stdout if not args.output else open(args.output, 'w')
        
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_strs = [f"{b:02X}" for b in chunk]
            out.write(" ".join(hex_strs) + "\n")
            
        if args.output:
            out.close()
            print(f"Wrote hex to {args.output}")

def main():
    parser = argparse.ArgumentParser(description="Convert images to bit sprites (1bpp, 2bpp, 4bpp, 8bpp)")
    parser.add_argument("input", help="Input image file")
    parser.add_argument("-o", "--output", help="Output file (default: stdout for text formats)")
    parser.add_argument("--bpp", type=int, choices=[1, 2, 4, 8], default=1, help="Bits per pixel (default: 1)")
    parser.add_argument("--threshold", type=int, default=128, help="Grayscale threshold for 1bpp (default: 128)")
    parser.add_argument("--invert", action="store_true", help="Invert pixel values")
    parser.add_argument("--width", type=int, help="Resize to width")
    parser.add_argument("--height", type=int, help="Resize to height")
    parser.add_argument("--format", choices=['asm', 'bin', 'hex'], default='asm', help="Output format (default: asm)")
    parser.add_argument("--label", default="sprite", help="Label name for assembly output (default: sprite)")
    parser.add_argument("--stride", type=int, help="Explicit stride in bytes per row (pads row with 0s to reach stride)")
    
    args = parser.parse_args()
    
    data = process_image(args)
    output_data(data, args)

if __name__ == "__main__":
    main()
