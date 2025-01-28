#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/ipv4-address-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("GpuNodesExample");

int main (int argc, char *argv[])
{
    // Enable logging
    LogComponentEnable ("16NodesExample", LOG_LEVEL_INFO);

    uint32_t numNodes = 16; // Number of nodes
    NodeContainer nodes;
    nodes.Create (numNodes); // Create 16 nodes

    // Create a PointToPoint link with 10Gbps speed and 1ms delay
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));

    // Create the devices and install them on the nodes
    NetDeviceContainer devices;
    for (uint32_t i = 0; i < numNodes - 1; i++)
    {
        NetDeviceContainer linkDevices = p2p.Install (nodes.Get (i), nodes.Get (i + 1));
        devices.Add (linkDevices);
    }

    // Install the internet stack (IP, TCP/UDP, etc.)
    InternetStackHelper stack;
    stack.Install (nodes);

    // Assign IP addresses to the devices
    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    address.Assign (devices);

    // Install simple UDP traffic (echo client-server)
    uint16_t port = 9;
    UdpEchoServerHelper echoServer (port);
    ApplicationContainer serverApps = echoServer.Install (nodes.Get (numNodes - 1));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (60.0));

    UdpEchoClientHelper echoClient (Ipv4Address ("10.1.1.15"), port);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (60.0));

    // Set up NetAnim for visualization and generate the XML file
    AnimationInterface anim ("16_nodes_netanim.xml");

    // Set up the simulation to run for 60 seconds
    Simulator::Stop (Seconds (60.0));
    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}

