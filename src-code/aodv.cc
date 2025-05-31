#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-module.h"
#include "myapp.h"

#include <fstream>
#include <cmath> // Thư viện toán học để tính sin, cos

NS_LOG_COMPONENT_DEFINE("AODV_TEST");

using namespace ns3;

// Khai báo các biến toàn cục
std::vector<Ptr<ConstantVelocityMobilityModel>> movers;
double position_interval = 1.0;

std::ofstream csvFile; // File CSV
Ptr<FlowMonitor> flowmon;
FlowMonitorHelper flowmonHelper;

// Hàm ghi thông số tại mỗi giây
void LogMetricsEverySecond()
{
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();

    double currentTime = Simulator::Now().GetSeconds();
    
    // In thông tin tất cả các flow để xác định các flow hiện có (chỉ trong 10 giây đầu)
    if (currentTime <= 10.0) {
        std::cout << "========== Thời điểm: " << currentTime << "s, Số lượng flow: " << stats.size() << " ==========" << std::endl;
        
        for (const auto& i : stats) {
            Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i.first);
            std::cout << "Flow " << i.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")" << std::endl;
        }
    }
    
    // Biến để tính toán thông số trung bình của tất cả các flow
    double totalThroughput = 0.0;
    double totalDelay = 0.0;
    double totalPdr = 0.0;
    int validFlowCount = 0;
    
    // Xử lý tất cả các flow
    for (const auto& i : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i.first);
        
        // Chỉ xử lý các flow có dữ liệu được truyền và nhận
        if (i.second.txPackets > 0 && i.second.rxPackets > 0) 
        {
            double timeFirstTxPacket = i.second.timeFirstTxPacket.GetSeconds();
            double timeLastRxPacket = i.second.timeLastRxPacket.GetSeconds();
            double duration = timeLastRxPacket - timeFirstTxPacket;
            
            // Đảm bảo chỉ tính khi thời gian hợp lệ
            if (duration > 0) 
            {
                double throughput = i.second.rxBytes * 8.0 / duration / 1024; // Kbps
                double avgDelay = i.second.delaySum.GetSeconds() / i.second.rxPackets; // seconds
                double pdr = (i.second.rxPackets * 100.0) / i.second.txPackets; // percentage
                
                // In thông tin chi tiết về flow (tùy chọn, có thể giữ lại hoặc bỏ)
                std::cout << "Flow " << i.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << "):" << std::endl;
                std::cout << "  Throughput: " << throughput << " Kbps" << std::endl;
                std::cout << "  Avg Delay:  " << avgDelay << " s" << std::endl;
                std::cout << "  PDR:        " << pdr << " %" << std::endl;
                
                // Cộng dồn cho giá trị trung bình
                totalThroughput += throughput;
                totalDelay += avgDelay;
                totalPdr += pdr;
                validFlowCount++;
            }
        }
    }
    
    // Tính giá trị trung bình cho tất cả các flow
    double avgThroughput = 0.0;
    double avgDelay = 0.0;
    double avgPdr = 0.0;
    
    if (validFlowCount > 0) {
        avgThroughput = totalThroughput / validFlowCount;
        avgDelay = totalDelay / validFlowCount;
        avgPdr = totalPdr / validFlowCount;
    }
    
    // In thông số trung bình của tất cả các flow
    std::cout << "========== THÔNG SỐ TRUNG BÌNH TẤT CẢ CÁC FLOW ==========" << std::endl;
    std::cout << "Thời điểm: " << currentTime << "s" << std::endl;
    std::cout << "Số flow hợp lệ: " << validFlowCount << std::endl;
    std::cout << "Throughput trung bình: " << avgThroughput << " Kbps" << std::endl;
    std::cout << "Delay trung bình: " << avgDelay << " s" << std::endl;
    std::cout << "PDR trung bình: " << avgPdr << " %" << std::endl;
    
    // Ghi dữ liệu vào file CSV
    csvFile << currentTime << "," << avgThroughput << "," << avgDelay << "," << avgPdr << "\n";
    csvFile.flush(); // Đảm bảo dữ liệu được ghi ngay lập tức
    
    // Lịch trình để ghi tiếp dữ liệu sau mỗi 1 giây
    if (currentTime < 99.0) {
        Simulator::Schedule(Seconds(1.0), &LogMetricsEverySecond);
    }
}

// Hàm dừng các node di chuyển
void stopMover()
{
    for (auto& mover : movers)
    {
        mover->SetVelocity(Vector(0, 0, 0));
    }
}

int main(int argc, char* argv[])
{
    csvFile.open("simulation_results_aodv.csv");
    csvFile << "Time,Throughput,Avg Delay,PDR\n"; // Tiêu đề cột

    bool enableFlowMonitor = true;
    std::string phyMode("DsssRate1Mbps");

    CommandLine cmd;
    cmd.AddValue("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
    cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
    cmd.Parse(argc, argv);

    // Tạo các node
    NS_LOG_INFO("Create nodes.");
    NodeContainer c;
    c.Create(40); // Tạo 40 nút

    // Cấu hình Wifi
    WifiHelper wifi;
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11);

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::ThreeLogDistancePropagationLossModel");

    wifiPhy.Set("TxPowerStart", DoubleValue(14));
    wifiPhy.Set("TxPowerEnd", DoubleValue(14));
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
        "DataMode", StringValue(phyMode),
        "ControlMode", StringValue(phyMode));

    NetDeviceContainer devices;
    devices = wifi.Install(wifiPhy, wifiMac, c);

    // Cấu hình AODV
    AodvHelper aodv;
    Ipv4ListRoutingHelper list;
    list.Add(aodv, 10);

    InternetStackHelper internet;
    internet.SetRoutingHelper(list);
    internet.Install(c);

    // Cài IP cho các node
    Ipv4AddressHelper ipv4;
    NS_LOG_INFO("Assign IP Addresses.");
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifcont = ipv4.Assign(devices);

    // Thiết lập sink trên tất cả các node
    uint16_t port = 9;
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = packetSinkHelper.Install(c);
    sinkApps.Start(Seconds(0.));
    sinkApps.Stop(Seconds(100.));

    // Đảm bảo vẫn giữ flow từ n0 đến n9 như yêu cầu ban đầu
    Ptr<Socket> ns3UdpSocket = Socket::CreateSocket(c.Get(0), UdpSocketFactory::GetTypeId());
    Address sinkAddress(InetSocketAddress(ifcont.GetAddress(9), port));
    
    Ptr<MyApp> app = CreateObject<MyApp>();
    app->Setup(ns3UdpSocket, sinkAddress, 1024, 3000, DataRate("150Kbps")); // 50000 packets
    c.Get(0)->AddApplication(app);
    
    app->SetStartTime(Seconds(1.));
    app->SetStopTime(Seconds(60.));
    
    
    // Tạo thêm các kết nối giữa các node để có nhiều flows
    UniformRandomVariable random;
    random.SetStream(10);
    
    // Tạo thêm các kết nối giữa các node để có nhiều flows
    for (uint32_t i = 1; i < 10; i++) { // 10 flows
        // Chọn ngẫu nhiên một node đích khác với node hiện tại
        uint32_t dest;
        do {
            dest = random.GetInteger(0, 39); // 40 node
        } while (dest == i);
        
        Ptr<Socket> socket = Socket::CreateSocket(c.Get(i), UdpSocketFactory::GetTypeId());
        Address destAddress(InetSocketAddress(ifcont.GetAddress(dest), port));
        
        Ptr<MyApp> newApp = CreateObject<MyApp>();
        newApp->Setup(socket, destAddress, 512, 3000, DataRate("250Kbps"));
        c.Get(i)->AddApplication(newApp);
        
        // Phân bố thời gian bắt đầu để tránh quá tải
        newApp->SetStartTime(Seconds(10.0 + 1.0 * i)); // Tăng interval và delay start time hơn
        newApp->SetStopTime(Seconds(60.0));
        
        std::cout << "Flow setup: Node " << i << " -> Node " << dest << std::endl;
    }

    // Đặt vị trí cho các nút
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    UniformRandomVariable randomX;
    randomX.SetStream(1);

    UniformRandomVariable randomY;
    randomY.SetStream(2);

    // Tạo vị trí ban đầu ngẫu nhiên cho các node
    for (uint32_t i = 0; i < 40; i++) {
        positionAlloc->Add(Vector(randomX.GetValue(0, 500), randomY.GetValue(0, 500), 0));//phạm vi mô phỏng là 500x500
    }

    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(c);

    // Thiết lập hướng di chuyển ngẫu nhiên với vận tốc cố định cho các node
    double fixedSpeed = 5.0; // Tốc độ cố định là 5 m/s
    UniformRandomVariable randomAngle; // Góc ngẫu nhiên
    randomAngle.SetStream(3);

    for (uint32_t i = 0; i < 40; i++) {
        Ptr<ConstantVelocityMobilityModel> moverModel = c.Get(i)->GetObject<ConstantVelocityMobilityModel>();
        
        // node 0 đứng yên
        if (i == 0) {
            moverModel->SetVelocity(Vector(0, 0, 0));
        }
        else {
            // Các node khác di chuyển ngẫu nhiên
            double angle = randomAngle.GetValue(0, 2 * M_PI); // Góc từ 0 đến 2π radian
            double velocityX = fixedSpeed * std::cos(angle); // Thành phần vận tốc theo trục x
            double velocityY = fixedSpeed * std::sin(angle); // Thành phần vận tốc theo trục y
            moverModel->SetVelocity(Vector(velocityX, velocityY, 0));
        }
        
        movers.push_back(moverModel);
    }

    // Cấu hình Flow Monitor
    flowmon = flowmonHelper.InstallAll();

    // Chạy mô phỏng
    NS_LOG_INFO("Run Simulation.");
    Simulator::Schedule(Seconds(1.0), &LogMetricsEverySecond); // Lịch trình ghi thông số mỗi giây
    Simulator::Schedule(Seconds(60), &stopMover); // Dừng di chuyển sau 60 giây
    Simulator::Stop(Seconds(100.)); // Dừng mô phỏng sau 100 giây
    Simulator::Run();

    // Đóng file CSV và kết thúc mô phỏng
    csvFile.close(); // Đóng file CSV
    Simulator::Destroy();
    NS_LOG_INFO("Done.");
    
    return 0;
}
