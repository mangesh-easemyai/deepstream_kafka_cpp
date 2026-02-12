import time
import os
import psutil
import pynvml
import sys

def clear_screen():
    # Clears the terminal screen
    os.system('cls' if os.name == 'nt' else 'clear')

def get_size(bytes, suffix="B"):
    """
    Scale bytes to its proper format
    e.g:
        1253656 => '1.20MB'
        1253656678 => '1.17GB'
    """
    factor = 1024
    for unit in ["", "K", "M", "G", "T", "P"]:
        if bytes < factor:
            return f"{bytes:.2f}{unit}{suffix}"
        bytes /= factor

def get_network_speed(old_net_io, new_net_io, interval):
    """Calculate network upload/download speed"""
    bytes_sent = new_net_io.bytes_sent - old_net_io.bytes_sent
    bytes_recv = new_net_io.bytes_recv - old_net_io.bytes_recv
    
    upload_speed = bytes_sent / interval
    download_speed = bytes_recv / interval
    
    return get_size(upload_speed) + "/s", get_size(download_speed) + "/s"

def print_dashboard():
    try:
        # Initialize NVIDIA Management Library
        pynvml.nvmlInit()
        device_count = pynvml.nvmlDeviceGetCount()
        gpu_active = True
    except:
        gpu_active = False
        print("⚠️  NVIDIA Driver not detected or NVML init failed.")

    # Initial network stats
    last_net_io = psutil.net_io_counters()
    last_time = time.time()

    print("🚀 Starting System Monitor... (Press Ctrl+C to Stop)")
    time.sleep(1)

    try:
        while True:
            # Calculate time delta for accurate network speed
            current_time = time.time()
            interval = current_time - last_time
            current_net_io = psutil.net_io_counters()
            
            # 1. CPU Stats
            cpu_percent = psutil.cpu_percent(interval=None)
            cpu_freq = psutil.cpu_freq()
            cpu_count = psutil.cpu_count(logical=False)
            
            # 2. Memory Stats
            svmem = psutil.virtual_memory()
            
            # 3. Network Stats
            upload, download = get_network_speed(last_net_io, current_net_io, interval)
            
            # Clear and Print
            clear_screen()
            print("="*60)
            print(f"📊  SYSTEM MONITOR  |  {time.strftime('%H:%M:%S')}")
            print("="*60)

            # CPU Section
            print(f"🔹 CPU Usage: {cpu_percent}%  |  Freq: {cpu_freq.current:.1f}Mhz  |  Cores: {cpu_count}")
            # Optional: Print per-core bar
            # print("-" * 40)

            # RAM Section
            print(f"🔹 RAM Usage: {svmem.percent}%  ({get_size(svmem.used)} / {get_size(svmem.total)})")

            # GPU Section
            if gpu_active:
                print("-" * 60)
                for i in range(device_count):
                    handle = pynvml.nvmlDeviceGetHandleByIndex(i)
                    name = pynvml.nvmlDeviceGetName(handle)
                    if isinstance(name, bytes): name = name.decode('utf-8')
                    
                    util = pynvml.nvmlDeviceGetUtilizationRates(handle)
                    mem = pynvml.nvmlDeviceGetMemoryInfo(handle)
                    temp = pynvml.nvmlDeviceGetTemperature(handle, pynvml.NVML_TEMPERATURE_GPU)
                    
                    print(f"🔹 GPU {i} [{name}]")
                    print(f"    ├── Load: {util.gpu}%")
                    print(f"    ├── Temp: {temp}°C")
                    print(f"    └── VRAM: {int(mem.used / 1024**2)}MB / {int(mem.total / 1024**2)}MB")

            # Network Section
            print("-" * 60)
            print(f"🔹 Network")
            print(f"    ├── Upload:   {upload}")
            print(f"    └── Download: {download}")
            print("="*60)

            # Update references
            last_net_io = current_net_io
            last_time = current_time
            
            # Refresh Rate (1 Second)
            time.sleep(1)

    except KeyboardInterrupt:
        print("\n🛑 Monitor Stopped.")
        if gpu_active:
            pynvml.nvmlShutdown()
        sys.exit(0)

if __name__ == "__main__":
    print_dashboard()