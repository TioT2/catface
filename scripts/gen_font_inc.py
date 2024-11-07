# 8x8 font .inc file generation script

import sys

def arr_chunks(arr, chunk_size):
    for i in range(0, len(arr), chunk_size):
        yield arr[i:i + chunk_size]

def gen_inc(barr):
    for row in arr_chunks(barr, 16):
        st = ''
        for n in row:
            st += f'0x{n:02X}, '
        print(st)

def main():
    if len(sys.argv) < 2:
        print('usage: python gen_font_inc.py <8x8 .fnt font file path>')
        return
    
    with open(sys.argv[1], 'br') as input:
        barr = bytearray(input.read())
        gen_inc(barr)

if __name__ == '__main__':
    main()
