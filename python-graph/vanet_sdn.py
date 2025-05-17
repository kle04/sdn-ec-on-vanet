import pandas as pd
import matplotlib.pyplot as plt

# Đọc dữ liệu từ file CSV
file_name = "simulation_results_sdn_vanet.csv"
data = pd.read_csv(file_name)

# Xử lý dữ liệu
time = data['Time']  # Cột thời gian
throughput = data['Throughput']  # Cột thông lượng (Mbps)
delay = data['Avg Delay']  # Độ trễ trung bình (seconds)
pdr = data['PDR']  # Tỷ lệ phân phối gói tin (%)

# Vẽ biểu đồ
plt.figure(figsize=(12, 10))

# Throughput
plt.subplot(3, 1, 1)
plt.bar(time, throughput, color='green', width=0.5)
plt.title('SDN-VANET - Throughput theo Time')
plt.xlabel('Time (s)')
plt.ylabel('Throughput (Mbps)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Delay
plt.subplot(3, 1, 2)
plt.bar(time, delay, color='blue', width=0.5)
plt.title('SDN-VANET - Avg Delay theo Time')
plt.xlabel('Time (s)')
plt.ylabel('Avg Delay (s)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

# PDR
plt.subplot(3, 1, 3)
plt.bar(time, pdr, color='red', width=0.5)
plt.title('SDN-VANET - PDR theo Time')
plt.xlabel('Time (s)')
plt.ylabel('PDR (%)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Hiển thị biểu đồ
plt.tight_layout()
plt.savefig('sdn_vanet_performance_graph.png')
plt.show() 
