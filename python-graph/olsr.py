import pandas as pd
import matplotlib.pyplot as plt

# Đọc dữ liệu từ file CSV
file_name = "Result/simulation_results_olsr.csv"
data = pd.read_csv(file_name)

# Xử lý dữ liệu
time = data['Time']  # Cột thời gian
throughput = data['Throughput']  # Cột thông lượng (Mbps)
delay = data['Avg Delay']  # Độ trễ trung bình (ms)
pdr = data['PDR']  # Tỷ lệ phân phối gói tin (%)

# Vẽ biểu đồ
plt.figure(figsize=(12, 10))

# Throughput
plt.subplot(3, 1, 1)
plt.bar(time, throughput, color='green', width=0.5)
plt.title('OLSR - Throughput theo Time')
plt.xlabel('Time (s)')
plt.ylabel('Throughput (Mbps)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Delay
plt.subplot(3, 1, 2)
plt.bar(time, delay, color='blue', width=0.5)
plt.title('OLSR - Avg Delay theo Time')
plt.xlabel('Time (s)')
plt.ylabel('Avg Delay (ms)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

# PDR
plt.subplot(3, 1, 3)
plt.bar(time, pdr, color='red', width=0.5)
plt.title('OLSR - PDR theo Time')
plt.xlabel('Time (s)')
plt.ylabel('PDR (%)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Hiển thị biểu đồ
plt.tight_layout()
plt.savefig('olsr_performance_graph.png')
plt.show()
