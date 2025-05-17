import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Đọc dữ liệu từ các file CSV
sdn_file = "Result/simulation_results_sdn_vanet.csv"
aodv_file = "Result/simulation_results_aodv.csv"
olsr_file = "Result/simulation_results_olsr.csv"

sdn_data = pd.read_csv(sdn_file)
aodv_data = pd.read_csv(aodv_file)
olsr_data = pd.read_csv(olsr_file)

# Đảm bảo dữ liệu được sắp xếp theo thời gian
sdn_data = sdn_data.sort_values(by='Time')
aodv_data = aodv_data.sort_values(by='Time')
olsr_data = olsr_data.sort_values(by='Time')

# Chuyển đổi dữ liệu Series sang mảng numpy để tránh lỗi multi-dimensional indexing
sdn_time = sdn_data['Time'].to_numpy()
sdn_throughput = sdn_data['Throughput'].to_numpy()
sdn_delay = sdn_data['Avg Delay'].to_numpy()
sdn_pdr = sdn_data['PDR'].to_numpy()

aodv_time = aodv_data['Time'].to_numpy()
aodv_throughput = aodv_data['Throughput'].to_numpy()
aodv_delay = aodv_data['Avg Delay'].to_numpy()
aodv_pdr = aodv_data['PDR'].to_numpy()

olsr_time = olsr_data['Time'].to_numpy()
olsr_throughput = olsr_data['Throughput'].to_numpy()
olsr_delay = olsr_data['Avg Delay'].to_numpy()
olsr_pdr = olsr_data['PDR'].to_numpy()

# Vẽ biểu đồ
plt.figure(figsize=(15, 15))

# 1. Throughput - Line Chart
plt.subplot(3, 1, 1)
plt.plot(sdn_time, sdn_throughput, marker='o', markersize=3, linewidth=2, color='blue', label='SDN')
plt.plot(aodv_time, aodv_throughput, marker='s', markersize=3, linewidth=2, color='red', label='AODV')
plt.plot(olsr_time, olsr_throughput, marker='^', markersize=3, linewidth=2, color='green', label='OLSR')

plt.title('Throughput Comparison', fontsize=14, fontweight='bold')
plt.xlabel('Time (s)')
plt.ylabel('Throughput (Kbps)')
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend()

# 2. Average Delay - Line Chart
plt.subplot(3, 1, 2)
plt.plot(sdn_time, sdn_delay, marker='o', markersize=3, linewidth=2, color='blue', label='SDN')
plt.plot(aodv_time, aodv_delay, marker='s', markersize=3, linewidth=2, color='red', label='AODV')
plt.plot(olsr_time, olsr_delay, marker='^', markersize=3, linewidth=2, color='green', label='OLSR')

plt.title('Average Delay Comparison', fontsize=14, fontweight='bold')
plt.xlabel('Time (s)')
plt.ylabel('Average Delay (s)')
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend()

# 3. PDR - Line Chart
plt.subplot(3, 1, 3)
plt.plot(sdn_time, sdn_pdr, marker='o', markersize=3, linewidth=2, color='blue', label='SDN')
plt.plot(aodv_time, aodv_pdr, marker='s', markersize=3, linewidth=2, color='red', label='AODV')
plt.plot(olsr_time, olsr_pdr, marker='^', markersize=3, linewidth=2, color='green', label='OLSR')

plt.title('Packet Delivery Ratio (PDR) Comparison', fontsize=14, fontweight='bold')
plt.xlabel('Time (s)')
plt.ylabel('PDR (%)')
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend()

# Hiển thị biểu đồ
plt.tight_layout()
plt.savefig('routing_protocols_comparison.png', dpi=300, bbox_inches='tight')

# Ngoài ra, tạo thêm biểu đồ cột cho giá trị trung bình
plt.figure(figsize=(15, 5))

# Tính giá trị trung bình
avg_throughput = [np.mean(sdn_throughput), np.mean(aodv_throughput), np.mean(olsr_throughput)]
avg_delay = [np.mean(sdn_delay), np.mean(aodv_delay), np.mean(olsr_delay)]
avg_pdr = [np.mean(sdn_pdr), np.mean(aodv_pdr), np.mean(olsr_pdr)]

protocols = ['SDN', 'AODV', 'OLSR']
x = np.arange(len(protocols))
width = 0.25

# Biểu đồ cột cho giá trị trung bình
plt.subplot(1, 3, 1)
plt.bar(x, avg_throughput, width, color=['blue', 'red', 'green'])
plt.xticks(x, protocols)
plt.title('Average Throughput')
plt.ylabel('Throughput (Kbps)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

plt.subplot(1, 3, 2)
plt.bar(x, avg_delay, width, color=['blue', 'red', 'green'])
plt.xticks(x, protocols)
plt.title('Average Delay')
plt.ylabel('Delay (s)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

plt.subplot(1, 3, 3)
plt.bar(x, avg_pdr, width, color=['blue', 'red', 'green'])
plt.xticks(x, protocols)
plt.title('Average PDR')
plt.ylabel('PDR (%)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

plt.tight_layout()
plt.savefig('routing_protocols_avg_comparison.png', dpi=300, bbox_inches='tight')

# In ra giá trị trung bình để tham khảo
print("=== AVERAGE PERFORMANCE METRICS ===")
print(f"Protocol  | Throughput (Kbps) | Delay (s) | PDR (%)")
print(f"SDN       | {avg_throughput[0]:.2f}           | {avg_delay[0]:.4f}    | {avg_pdr[0]:.2f}")
print(f"AODV      | {avg_throughput[1]:.2f}           | {avg_delay[1]:.4f}    | {avg_pdr[1]:.2f}")
print(f"OLSR      | {avg_throughput[2]:.2f}           | {avg_delay[2]:.4f}    | {avg_pdr[2]:.2f}")

plt.show() 
