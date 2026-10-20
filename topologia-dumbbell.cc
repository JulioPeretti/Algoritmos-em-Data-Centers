#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h" // Netanim pra visualizar

using namespace ns3;

int main(int argc, char *argv[])
{
    // Mantendo o TCP NewReno
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));

    uint32_t numLeft = 3;
    uint32_t numRight = 3;

    PointToPointHelper pointToPointLeaf;
    pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPointLeaf.SetChannelAttribute("Delay", StringValue("1ms"));

    PointToPointHelper pointToPointRouter;
    pointToPointRouter.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPointRouter.SetChannelAttribute("Delay", StringValue("10ms"));
    pointToPointRouter.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(5)); // buffer menor pra forçar gargalo
    //  Se fosse em C teria que fazer um monte de for pra ligar os cabos nas portas
    PointToPointDumbbellHelper dumbbell(numLeft, pointToPointLeaf, numRight, pointToPointLeaf, pointToPointRouter);

    InternetStackHelper stack;
    dumbbell.InstallStack(stack); // Instala o TCP/IP nos computadores

    dumbbell.AssignIpv4Addresses(
        Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),  // Computadores da esquerda
        Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),  // Computadores da direita
        Ipv4AddressHelper("10.3.1.0", "255.255.255.0")); // Roteadores do centro

    uint16_t port = 5000;
    for (uint32_t i = 0; i < numRight; ++i)
    {
        PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port)); // escuta os pacotes de qualquer ip (GetAny) anota e joga fora
        ApplicationContainer serverApp = sink.Install(dumbbell.GetRight(i));                            // faz isso pros 3 computadores
        serverApp.Start(Seconds(1.0));
        serverApp.Stop(Seconds(8.0));

        BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(dumbbell.GetRightIpv4Address(i), port));
        source.SetAttribute("MaxBytes", UintegerValue(0)); // Esse max bytes faz com que ele dispare pacotes sem parar
        ApplicationContainer clientApp = source.Install(dumbbell.GetLeft(i));
        clientApp.Start(Seconds(2.0));
        clientApp.Stop(Seconds(8.0));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Instalação do FlowMonitor pra ver XML
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Configuração do NetAnim ( linha 60 - 77)
    AnimationInterface anim("animacao-dumbbell.xml");

    anim.SetConstantPosition(dumbbell.GetLeft(), 30.0, 50.0);
    anim.SetConstantPosition(dumbbell.GetRight(), 70.0, 50.0);
    anim.UpdateNodeDescription(dumbbell.GetLeft(), "Roteador-Esq");
    anim.UpdateNodeDescription(dumbbell.GetRight(), "Roteador-Dir");

    for (uint32_t i = 0; i < numLeft; ++i)
    {
        anim.SetConstantPosition(dumbbell.GetLeft(i), 10.0, 30.0 + (i * 20.0));
        anim.UpdateNodeDescription(dumbbell.GetLeft(i), "Cliente-" + std::to_string(i));
    }
    for (uint32_t i = 0; i < numRight; ++i)
    {
        anim.SetConstantPosition(dumbbell.GetRight(i), 90.0, 30.0 + (i * 20.0));
        anim.UpdateNodeDescription(dumbbell.GetRight(i), "Servidor-" + std::to_string(i));
    }

    Simulator::Stop(Seconds(8.0));

    Simulator::Run();

    // Exporta o XML
    monitor->SerializeToXmlFile("resultados-flowmonitor.xml", true, true);

    Simulator::Destroy();

    return 0;
}