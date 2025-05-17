#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/olsr-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ofswitch13-module.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include "myappsdn.h" // Include class MyApp từ file riêng

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("VanetSdn2RSUExample");

// Khai báo vector lưu trữ các node di chuyển
std::vector<Ptr<ConstantVelocityMobilityModel>> movers;

// Khai báo các biến theo dõi hiệu suất
std::ofstream csvFile; // File CSV để lưu kết quả
Ptr<FlowMonitor> flowMonitor;
FlowMonitorHelper flowHelper;
Ipv4InterfaceContainer allWirelessInterfaces; // Di chuyển ra ngoài để trở thành biến toàn cục

// Khai báo hằng số khoảng cách kết nối tối đa - giảm xuống để thực tế hơn
const double MAX_V2V_DISTANCE = 15.0; // Giảm khoảng cách V2V từ 100m xuống 15m
const double MAX_V2I_DISTANCE = 20.0; // Giảm khoảng cách V2I từ 150m xuống 20m

// Hàm tính khoảng cách giữa hai điểm
double MyCalculateDistance(const Vector& a, const Vector& b)
{
  return std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2));
}

// Hàm dừng các node di chuyển
void stopMover()
{
    for (auto& mover : movers)
    {
        mover->SetVelocity(Vector(0, 0, 0));
    }
}

// Hàm tính hướng di chuyển hướng về RSU
Vector calculateVelocityTowardsRSU(const Vector& vehiclePos, const Vector& rsuPos, double speed)
{
    // Tính vector hướng từ xe đến RSU
    Vector direction(rsuPos.x - vehiclePos.x, rsuPos.y - vehiclePos.y, 0);
    
    // Tính chiều dài của vector hướng
    double length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    
    // Nếu xe đã ở gần RSU, cho phép di chuyển ngẫu nhiên hơn
    if (length < 50) {
        // Thêm một thành phần ngẫu nhiên nhỏ - sử dụng random toàn cục thay vì cục bộ
        static UniformRandomVariable random;
        static bool initialized = false;
        if (!initialized) {
            random.SetStream(5); // Sử dụng stream 5 (chưa được sử dụng trước đó)
            initialized = true;
        }
        double randomAngle = random.GetValue(0, 2 * M_PI);
        return Vector(speed * std::cos(randomAngle), speed * std::sin(randomAngle), 0);
    }
    
    // Chuẩn hóa vector hướng
    direction.x = direction.x / length * speed;
    direction.y = direction.y / length * speed;
    
    return direction;
}

// Hàm ghi thông số mạng vào file CSV
void LogMetricsEverySecond()
{
    // Đối tượng để phân loại flow
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

    double currentTime = Simulator::Now().GetSeconds();
    
    // In thông tin tất cả các flow để xác định các flow hiện có
    if (currentTime <= 10.0) { // Chỉ in trong 10 giây đầu để xác định flow
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
    
    // Lên lịch cho lần ghi tiếp theo (mỗi 1 giây)
    if (currentTime < 99.0)
    {
        Simulator::Schedule(Seconds(1.0), &LogMetricsEverySecond);
    }
}

int 
main (int argc, char *argv[])
{
  // Mở file CSV để lưu kết quả - đổi tên file đầu ra để phản ánh cấu hình 2 RSU
  csvFile.open("simulation_results_sdn_vanet_2rsu.csv");
  csvFile << "Time,Throughput,Avg Delay,PDR\n"; // Tiêu đề cột

  // Enable logging
  LogComponentEnable ("VanetSdn2RSUExample", LOG_LEVEL_INFO);
  LogComponentEnable ("OFSwitch13Device", LOG_LEVEL_INFO);
  LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_INFO);

  // Xử lý tham số từ command line
  bool enableFlowMonitor = true;
  
  CommandLine cmd;
  cmd.AddValue("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
  cmd.Parse(argc, argv);
  
  // Xóa các thiết lập cấu hình PCAP để khắc phục lỗi
  // Config::SetDefault("ns3::PcapFileWrapper::CaptureSize", UintegerValue(65535));
  // Config::SetDefault("ns3::PcapFileWrapper::MaxPcapPackets", UintegerValue(1000000));

  // Create nodes - giảm số lượng node phương tiện để giảm tải
  NodeContainer vehNodes;
  vehNodes.Create(50);  // Giảm từ 50 xuống 40 xe để giảm tải
  NodeContainer rsuNodes;
  rsuNodes.Create(2);   // Giảm từ 4 xuống 2 RSU theo yêu cầu
  NodeContainer switchNodes;
  switchNodes.Create(1);     // 1 switch OpenFlow
  NodeContainer controllerNodes;
  controllerNodes.Create(1); // 1 controller SDN
  Ptr<Node> serverNode = CreateObject<Node>();     // 1 server trung tâm

  // Cài đặt giao thức Internet (TCP/IP) cho các node trừ switch
  InternetStackHelper internet;
  
  // Thêm giao thức định tuyến OLSR - thích hợp cho mạng ad-hoc VANET
  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper list;
  list.Add(staticRouting, 0);
  list.Add(olsr, 10);  // OLSR có ưu tiên cao hơn
  
  internet.SetRoutingHelper(list);
  internet.Install(vehNodes);
  internet.Install(rsuNodes);
  internet.Install(serverNode);
  internet.Install(controllerNodes);

  // Thiết lập mobility cho xe
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

  // Tạo vị trí ban đầu ngẫu nhiên cho các xe trong không gian 500x500m
  UniformRandomVariable randomX;
  randomX.SetStream(1);
  UniformRandomVariable randomY;
  randomY.SetStream(2);
  UniformRandomVariable rsuSelector;
  rsuSelector.SetStream(4);
  
  // Định nghĩa trước vị trí cho 2 RSU
  // Định vị 2 RSU trong không gian 500x500m (thay vì 4 RSU)
  Vector rsu0Pos(125.0, 250.0, 0.0);  // RSU 1 ở phía Tây
  Vector rsu1Pos(375.0, 250.0, 0.0);  // RSU 2 ở phía Đông
  
  std::vector<Vector> rsuPositions = {rsu0Pos, rsu1Pos};
  
  for (uint32_t i = 0; i < 40; i++) {  // 40 xe thay vì 50 xe
      // Chọn một RSU để tạo xe gần đó
      int selectedRsu = rsuSelector.GetInteger(0, 1); // Chỉ chọn từ 2 RSU (0 hoặc 1)
      Vector rsuPos = rsuPositions[selectedRsu];
      
      // Tạo vị trí xe trong phạm vi 125m quanh RSU được chọn
      double angle = randomX.GetValue(0, 2 * M_PI);
      double distance = randomY.GetValue(25, 125);
      double xPos = rsuPos.x + distance * std::cos(angle);
      double yPos = rsuPos.y + distance * std::sin(angle);
      
      // Giới hạn trong môi trường mô phỏng 500x500m
      xPos = std::max(0.0, std::min(500.0, xPos));
      yPos = std::max(0.0, std::min(500.0, yPos));
      
      positionAlloc->Add(Vector(xPos, yPos, 0));
  }

  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
  mobility.Install(vehNodes);

  // Thiết lập hướng di chuyển ưu tiên hướng về RSU với vận tốc 5m/s theo yêu cầu
  double fixedSpeed = 5.0; // Tốc độ 5 m/s theo yêu cầu
  UniformRandomVariable randomRsu;
  randomRsu.SetStream(3);

  for (uint32_t i = 0; i < 40; i++) { // 40 xe thay vì 50 xe
      Ptr<ConstantVelocityMobilityModel> moverModel = vehNodes.Get(i)->GetObject<ConstantVelocityMobilityModel>();
      Vector position = moverModel->GetPosition();
      
      // Chọn RSU mục tiêu (có thể là RSU gần nhất hoặc chọn ngẫu nhiên)
      Vector targetRsu;
      if (randomRsu.GetValue(0, 1) < 0.7) {
          // 70% trường hợp, chọn RSU gần nhất
          double minDist = std::numeric_limits<double>::max();
          for (const auto& rsuPos : rsuPositions) {
              double dist = std::pow(position.x - rsuPos.x, 2) + std::pow(position.y - rsuPos.y, 2);
              if (dist < minDist) {
                  minDist = dist;
                  targetRsu = rsuPos;
              }
          }
      } else {
          // 30% trường hợp, chọn RSU ngẫu nhiên
          int randRsuIndex = randomRsu.GetInteger(0, 1); // Chỉ chọn từ 2 RSU
          targetRsu = rsuPositions[randRsuIndex];
      }
      
      // Tính vector vận tốc hướng về RSU
      Vector velocity = calculateVelocityTowardsRSU(position, targetRsu, fixedSpeed);
      moverModel->SetVelocity(velocity);
      movers.push_back(moverModel);
  }

  // Set vị trí RSU mặc định
  MobilityHelper mobilityRsu;
  mobilityRsu.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  Ptr<ListPositionAllocator> rsuPositionsAlloc = CreateObject<ListPositionAllocator>();
  for (const auto& rsuPos : rsuPositions) {
      rsuPositionsAlloc->Add(rsuPos);
  }
  mobilityRsu.SetPositionAllocator(rsuPositionsAlloc);
  mobilityRsu.Install(rsuNodes);

  // Thiết lập vị trí cho switch và controller
  MobilityHelper mobilityStatic;
  mobilityStatic.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  Ptr<ListPositionAllocator> staticPositions = CreateObject<ListPositionAllocator>();
  staticPositions->Add(Vector(250.0, 250.0, 0.0));  // Switch ở giữa
  staticPositions->Add(Vector(250.0, 300.0, 0.0));  // Controller
  staticPositions->Add(Vector(250.0, 200.0, 0.0));  // Server
  mobilityStatic.SetPositionAllocator(staticPositions);
  mobilityStatic.Install(switchNodes);
  mobilityStatic.Install(controllerNodes);
  mobilityStatic.Install(serverNode);

  // Thiết lập kênh truyền thông với khoảng cách ngắn hơn
  YansWifiChannelHelper channel;
  channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                            "Exponent", DoubleValue(3.5), // Tăng hệ số suy giảm (từ 2.5 lên 3.5)
                            "ReferenceDistance", DoubleValue(1.0),
                            "ReferenceLoss", DoubleValue(46.0)); // Tăng suy hao cơ bản (từ 40.0 lên 46.0)

  YansWifiPhyHelper phy;
  phy.SetChannel(channel.Create());
  phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11);
  
  // Giảm công suất phát để giảm phạm vi truyền thông
  phy.Set("TxPowerStart", DoubleValue(16.0)); // Giảm từ 30.0 xuống 16.0 dBm
  phy.Set("TxPowerEnd", DoubleValue(16.0));   // Giảm từ 30.0 xuống 16.0 dBm
  phy.Set("TxGain", DoubleValue(0.0));        // Giảm từ 12.0 xuống 0.0 dBi
  phy.Set("RxGain", DoubleValue(0.0));        // Giảm từ 12.0 xuống 0.0 dBi
  phy.Set("RxSensitivity", DoubleValue(-80.0)); // Giảm độ nhạy từ -85.0 xuống -80.0 dBm

  // Cấu hình MAC cho mạng Ad-hoc
  WifiMacHelper mac;
  mac.SetType("ns3::AdhocWifiMac", 
             "QosSupported", BooleanValue(true));

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211p);  // IEEE 802.11p for VANET
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                              "DataMode", StringValue("OfdmRate12MbpsBW10MHz"),
                              "ControlMode", StringValue("OfdmRate6MbpsBW10MHz"));

  // Tạo các interface mạng
  NetDeviceContainer vehDevices = wifi.Install(phy, mac, vehNodes);
  NetDeviceContainer rsuDevices = wifi.Install(phy, mac, rsuNodes);
  
  // Thiết lập địa chỉ IP
  Ipv4AddressHelper ipv4;
  
  // Một dải mạng IP duy nhất cho tất cả các xe và RSU 
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  allWirelessInterfaces.Add(ipv4.Assign(vehDevices));
  allWirelessInterfaces.Add(ipv4.Assign(rsuDevices));
  
  // Thiết lập OFSwitch13
  Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper>();
  Ptr<OFSwitch13LearningController> controller = CreateObject<OFSwitch13LearningController>();
  of13Helper->InstallController(controllerNodes.Get(0), controller);

  // Cài đặt OFSwitch13 device trên switch node
  NetDeviceContainer switchPorts;
  of13Helper->InstallSwitch(switchNodes.Get(0), switchPorts);

  // Kết nối các node với switch
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));

  // Kết nối RSU với switch - chỉ 2 RSU
  for (uint32_t i = 0; i < rsuNodes.GetN(); ++i)
  {
    NetDeviceContainer link = p2p.Install(rsuNodes.Get(i), switchNodes.Get(0));
    switchPorts.Add(link.Get(1));
    // Sử dụng subnet khác nhau cho mỗi kết nối RSU-switch
    std::string base = "10.1." + std::to_string(i + 3) + ".0";
    ipv4.SetBase(base.c_str(), "255.255.255.0");
    ipv4.Assign(link);
  }

  // Kết nối server với switch
  NetDeviceContainer serverLink = p2p.Install(serverNode, switchNodes.Get(0));
  switchPorts.Add(serverLink.Get(1));
  ipv4.SetBase("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer serverInterface = ipv4.Assign(serverLink);

  // Cài đặt OFSwitch13 helper
  of13Helper->CreateOpenFlowChannels();

  // Thiết lập ứng dụng server
  uint16_t port = 9;
  PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
  ApplicationContainer serverApps = packetSinkHelper.Install(serverNode);
  serverApps.Start(Seconds(1.0));
  serverApps.Stop(Seconds(99.0));

  // Thiết lập các ứng dụng gửi dữ liệu từ xe đến server - giảm số lượng
  for (uint32_t i = 0; i < 10; i++) {  // Giảm từ 15 xuống 10 xe gửi đến server
    Ptr<Socket> ns3UdpSocket = Socket::CreateSocket(vehNodes.Get(i), UdpSocketFactory::GetTypeId());
    Address serverAddress(InetSocketAddress(serverInterface.GetAddress(0), port));
    
    Ptr<MyApp> app = CreateObject<MyApp>();
    app->Setup(ns3UdpSocket, serverAddress, 1024, 5000, DataRate("250Kbps")); // 250Kbps theo yêu cầu
    vehNodes.Get(i)->AddApplication(app);
    
    app->SetStartTime(Seconds(2.0 + i * 0.1));
    app->SetStopTime(Seconds(95.0));
  }
  
  // Thiết lập flow trực tiếp từ node0 đến node9 (theo yêu cầu) 
  uint16_t directPort = 5678;
  
  // Thiết lập sink trên tất cả các phương tiện để có thể nhận gói tin
  PacketSinkHelper directSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), directPort));
  ApplicationContainer directSinkApp = directSinkHelper.Install(vehNodes);
  directSinkApp.Start(Seconds(1.0));
  directSinkApp.Stop(Seconds(95.0));
  
  // Tạo flows giữa các phương tiện gần nhau
  for (uint32_t i = 0; i < vehNodes.GetN(); i++) {
    Ptr<MobilityModel> senderMobility = vehNodes.Get(i)->GetObject<MobilityModel>();
    
    // Đảm bảo node0 luôn truyền đến node9 (theo yêu cầu ban đầu)
    if (i == 0) {
      Ptr<Socket> directSocket = Socket::CreateSocket(vehNodes.Get(i), UdpSocketFactory::GetTypeId());
      Address directAddress(InetSocketAddress(allWirelessInterfaces.GetAddress(9), directPort));
      
      Ptr<MyApp> directApp = CreateObject<MyApp>();
      directApp->Setup(directSocket, directAddress, 1024, 10000, DataRate("250Kbps"));
      vehNodes.Get(i)->AddApplication(directApp);
      
      directApp->SetStartTime(Seconds(2.0));
      directApp->SetStopTime(Seconds(95.0));
      
      std::cout << "Direct flow setup: Vehicle " << i << " -> Vehicle 9" << std::endl;
    }
    
    // Tìm tất cả các node trong phạm vi của node hiện tại
    for (uint32_t j = 0; j < vehNodes.GetN(); j++) {
      if (i == j) continue; // Không truyền đến chính nó
      
      Ptr<MobilityModel> receiverMobility = vehNodes.Get(j)->GetObject<MobilityModel>();
      double distance = senderMobility->GetDistanceFrom(receiverMobility);
      
      // Nếu trong phạm vi V2V thì tạo kết nối
      // Sử dụng ngưỡng lớn hơn MAX_V2V_DISTANCE để đảm bảo có đủ kết nối trong simulation
      if (distance <= 30.0) { // Tăng lên 30m để đảm bảo có đủ kết nối
        Ptr<Socket> socket = Socket::CreateSocket(vehNodes.Get(i), UdpSocketFactory::GetTypeId());
        Address receiverAddress(InetSocketAddress(allWirelessInterfaces.GetAddress(j), directPort));
        
        Ptr<MyApp> app = CreateObject<MyApp>();
        app->Setup(socket, receiverAddress, 512, 1000, DataRate("250Kbps"));
        vehNodes.Get(i)->AddApplication(app);
        
        // Phân bố thời gian bắt đầu để tránh quá tải
        app->SetStartTime(Seconds(5.0 + 0.02 * i * j));
        app->SetStopTime(Seconds(95.0));
        
        std::cout << "Flow setup: Vehicle " << i << " -> Vehicle " << j 
                  << ", Distance: " << distance << "m" << std::endl;
      }
    }
  }
  
  // Thiết lập animation
  AnimationInterface anim("vanet-sdn.xml");
  anim.SetConstantPosition(switchNodes.Get(0), 250, 250, 0);
  anim.SetConstantPosition(controllerNodes.Get(0), 250, 300, 0);
  anim.SetConstantPosition(serverNode, 250, 200, 0);

  // Cài đặt Flow Monitor để theo dõi hiệu suất mạng
  if (enableFlowMonitor) {
    flowMonitor = flowHelper.InstallAll();
    flowMonitor->SetAttribute("DelayBinWidth", DoubleValue(0.001));
    flowMonitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
    flowMonitor->SetAttribute("PacketSizeBinWidth", DoubleValue(20));
  }

  // Lên lịch ghi thông số mạng
  Simulator::Schedule(Seconds(5.0), &LogMetricsEverySecond);

  // Lên lịch dừng di chuyển sau 60 giây theo yêu cầu
  Simulator::Schedule(Seconds(60.0), &stopMover);

  // Chạy mô phỏng
  Simulator::Stop(Seconds(100.0));
  Simulator::Run();
  
  // Kết thúc mô phỏng
  csvFile.close(); // Đóng file CSV trước khi kết thúc
  Simulator::Destroy();
  
  return 0;
}
