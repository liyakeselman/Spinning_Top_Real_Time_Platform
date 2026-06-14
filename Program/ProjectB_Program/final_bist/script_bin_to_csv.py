import struct
import csv

# I=uint32, B=uint8, f=float, i=int32
STRUCT_FORMAT = "<I B f f f i i f f f f f f f f f f f f f f f i I B B B B B B"
STRUCT_SIZE = 99
HEADER = [
    # general
    "iter_global", "activityType", 
    "raw_data_samp_time[ms]", "binLineWr_time[ms]", "iterTime_binlineIdx[ms]",
    # luna
    "distLuna [cm]", "strengthLuna[Amp]", "tempLuna[Cdeg]",
    # mpu
    "mpuAngle[deg]", "mpuTotGyro[rad/sec]", "mpuTotAcc[m/s^2]", 
    "mpu_gyroX[rad/sec]", "mpu_gyroY[rad/sec]", "mpu_GyroZ[rad/sec]", 
    "mpu_accX[m/s^2]", "mpu_accY[m/s^2]", "mpu_accZ[m/s^2]",
    # VLs
    "vl1_dist[cm]", "vl2_dist[cm]", "vl3_dist[cm]", "VLAngle[deg]",
    # rotation
    "fusedAngle[deg]", "spincount", "bucket10", "digit",
    # hammer and top
    "currentTopDir", "currentHammerDir", "currentSystemMode", 
    "currentTopMode", "currentHammerMode"
]

def convert_bin_to_csv(bin_filename, csv_filename):
    print(f"Starting conversion: {bin_filename} -> {csv_filename}")
    
    count = 0
    try:
        with open(bin_filename, "rb") as bin_file, open(csv_filename, "w", newline="") as csv_file:
            writer = csv.writer(csv_file)
            # write title row
            writer.writerow(HEADER)
            
            while True:
                # read one packet (99 bytes)
                chunk = bin_file.read(STRUCT_SIZE)
                if len(chunk) < STRUCT_SIZE:
                    break
                
                # binaric decode to values list
                data = struct.unpack(STRUCT_FORMAT, chunk)
                # display only 3 digits after the point
                formatted_data = [f"{val:.3f}" if isinstance(val, float) else val for val in data]
                writer.writerow(formatted_data)
                count += 1
                
        print(f"Success! Processed {count} samples.")
        
    except FileNotFoundError:
        print(f"Error: The file '{bin_filename}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

# run
if __name__ == "__main__":
    input_file = "sensors_data.bin" 
    output_file = "sensors_data_final.csv"
    convert_bin_to_csv(input_file, output_file)