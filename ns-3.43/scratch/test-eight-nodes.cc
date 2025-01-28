#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;

int main(int argc, char *argv[]) {

    CommandLine cmd;
    cmd.Parse(argc, argv);

    // Create 8 nodes
    NodeContainer nodes;
    nodes.Create(8);

    // Create point-to-point links between nodes (a ring topology)
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    // Install devices and assign links in a ring
    NetDeviceContainer devices[8];
    for (int i = 0; i < 8; ++i) {
        devices[i] = pointToPoint.Install(NodeContainer(nodes.Get(i), nodes.Get((i + 1) % 8)));
    }

    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    char subnet[20];
    Ipv4InterfaceContainer interfaces[8];
    for (int i = 0; i < 8; ++i) {
        snprintf(subnet, sizeof(subnet), "10.1.%d.0", i + 1);
        address.SetBase(subnet, "255.255.255.0");
        interfaces[i] = address.Assign(devices[i]);
    }

    // Install a packet sink on the first node
    uint16_t port = 9; // Well-known echo port
    Address sinkAddress(InetSocketAddress(interfaces[0].GetAddress(0), port));
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", sinkAddress);
    ApplicationContainer sinkApp = packetSinkHelper.Install(nodes.Get(0));
    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(20.0));

    // Install UDP echo clients on other nodes to send traffic to node 0
    for (int i = 1; i < 8; ++i) {
        UdpEchoClientHelper echoClient(interfaces[0].GetAddress(0), port);
        echoClient.SetAttribute("MaxPackets", UintegerValue(5));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        echoClient.SetAttribute("PacketSize", UintegerValue(1024));

        ApplicationContainer clientApp = echoClient.Install(nodes.Get(i));
        clientApp.Start(Seconds(2.0 + i * 0.5)); // Stagger traffic start times
        clientApp.Stop(Seconds(20.0));
    }

    // Enable NetAnim tracing
    AnimationInterface anim("eight_node_netanim.xml");

    // Add descriptions to nodes for NetAnim
    for (uint32_t i = 0; i < nodes.GetN(); ++i) {
        anim.UpdateNodeDescription(nodes.Get(i), "Node " + std::to_string(i));
        anim.UpdateNodeColor(nodes.Get(i), 0, 255, 0); 
    }

    // Run simulation
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
