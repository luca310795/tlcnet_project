/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/* *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Michele Polese <michele.polese@gmail.com>
 */

#include "ns3/mmwave-helper.h"
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/netanim-module.h"
//#include "ns3/gtk-config-store.h"
#include <ns3/buildings-helper.h>
#include <ns3/buildings-module.h>
#include <ns3/random-variable-stream.h>
#include <ns3/lte-ue-net-device.h>


#include <iostream>
#include <ctime>
#include <stdlib.h>
#include <list>


using namespace ns3;

/**
 * Sample simulation script for MC device. It instantiates a LTE and two MmWave eNodeB,
 * attaches one MC UE to both and starts a flow for the UE to and from a remote host.
 */

NS_LOG_COMPONENT_DEFINE ("McTwoEnbs");

void 
PrintGnuplottableBuildingListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  uint32_t index = 0;
  for (BuildingList::Iterator it = BuildingList::Begin (); it != BuildingList::End (); ++it)
    {
      ++index;
      Box box = (*it)->GetBoundaries ();
      outFile << "set object " << index
              << " rect from " << box.xMin  << "," << box.yMin
              << " to "   << box.xMax  << "," << box.yMax
              << " front fs empty "
              << std::endl;
    }
}

void 
PrintGnuplottableUeListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteUeNetDevice> uedev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
          Ptr<MmWaveUeNetDevice> mmuedev = node->GetDevice (j)->GetObject <MmWaveUeNetDevice> ();
          Ptr<McUeNetDevice> mcuedev = node->GetDevice (j)->GetObject <McUeNetDevice> ();
          if (uedev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << uedev->GetImsi ()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps 0.3 lc rgb \"black\" offset 0,0"
                      << std::endl;
            }
          else if (mmuedev)
           {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << mmuedev->GetImsi ()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps 0.3 lc rgb \"black\" offset 0,0"
                      << std::endl;
            }
          else if (mcuedev)
           {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << mcuedev->GetImsi ()
                      << "\" at "<< pos.x << "," << pos.y << " left font \"Helvetica,8\" textcolor rgb \"black\" front point pt 1 ps 0.3 lc rgb \"black\" offset 0,0"
                      << std::endl;
            } 
        }
    }
}

void 
PrintGnuplottableEnbListToFile (std::string filename)
{
  std::ofstream outFile;
  outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::trunc);
  if (!outFile.is_open ())
    {
      NS_LOG_ERROR ("Can't open file " << filename);
      return;
    }
  for (NodeList::Iterator it = NodeList::Begin (); it != NodeList::End (); ++it)
    {
      Ptr<Node> node = *it;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; j++)
        {
          Ptr<LteEnbNetDevice> enbdev = node->GetDevice (j)->GetObject <LteEnbNetDevice> ();
          Ptr<MmWaveEnbNetDevice> mmdev = node->GetDevice (j)->GetObject <MmWaveEnbNetDevice> ();
          if (enbdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << enbdev->GetCellId ()
                      << "\" at "<< pos.x << "," << pos.y
                      << " left font \"Helvetica,8\" textcolor rgb \"blue\" front  point pt 4 ps 0.3 lc rgb \"blue\" offset 0,0"
                      << std::endl;
            }
          else if (mmdev)
            {
              Vector pos = node->GetObject<MobilityModel> ()->GetPosition ();
              outFile << "set label \"" << mmdev->GetCellId ()
                      << "\" at "<< pos.x << "," << pos.y
                      << " left font \"Helvetica,8\" textcolor rgb \"red\" front  point pt 4 ps 0.3 lc rgb \"red\" offset 0,0"
                      << std::endl;
            } 
        }
    }
}

void
PrintPosition(Ptr<Node> node)
{
  Ptr<MobilityModel> model = node->GetObject<MobilityModel> ();
  NS_LOG_UNCOND("Position +****************************** " << model->GetPosition() << " at time " << Simulator::Now().GetSeconds());
}

void 
PrintLostUdpPackets(Ptr<UdpServer> app, std::string fileName)
{
  std::ofstream logFile(fileName.c_str(), std::ofstream::app);
  logFile << Simulator::Now().GetSeconds() << " " << app->GetLost() << std::endl;
  logFile.close();
  Simulator::Schedule(MilliSeconds(20), &PrintLostUdpPackets, app, fileName);
}

static ns3::GlobalValue g_interPckInterval("interPckInterval", "Interarrival time of UDP packets (us)",
    ns3::UintegerValue(1), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_bufferSize("bufferSize", "RLC tx buffer size (MB)",
    ns3::UintegerValue(20), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_x2Latency("x2Latency", "Latency on X2 interface (us)",
    ns3::DoubleValue(500), ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_mmeLatency("mmeLatency", "Latency on MME interface (us)",
    ns3::DoubleValue(10000), ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_fastSwitching("fastSwitching", "If true, use mc setup, else use hard handover",
    ns3::BooleanValue(true), ns3::MakeBooleanChecker());
static ns3::GlobalValue g_rlcAmEnabled("rlcAmEnabled", "If true, use RLC AM, else use RLC UM",
    ns3::BooleanValue(true), ns3::MakeBooleanChecker());
static ns3::GlobalValue g_runNumber ("runNumber", "Run number for rng",
    ns3::UintegerValue(10), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_outPath("outPath",
    "The path of output log files",
    ns3::StringValue("./"), ns3::MakeStringChecker());
static ns3::GlobalValue g_noiseAndFilter("noiseAndFilter", "If true, use noisy SINR samples, filtered. If false, just use the SINR measure",
    ns3::BooleanValue(false), ns3::MakeBooleanChecker());
static ns3::GlobalValue g_handoverMode("handoverMode",
    "Handover mode",
    ns3::UintegerValue(3), ns3::MakeUintegerChecker<uint8_t>());
static ns3::GlobalValue g_reportTablePeriodicity("reportTablePeriodicity", "Periodicity of RTs",
    ns3::UintegerValue(25600), ns3::MakeUintegerChecker<uint32_t>());
static ns3::GlobalValue g_outageThreshold("outageTh", "Outage threshold",
    ns3::DoubleValue(-5), ns3::MakeDoubleChecker<double>());
static ns3::GlobalValue g_lteUplink("lteUplink", "If true, always use LTE for uplink signalling",
    ns3::BooleanValue(false), ns3::MakeBooleanChecker());

int
main (int argc, char *argv[])
{
  //LogComponentEnable ("LteEnbRrc", LOG_LEVEL_ERROR);

  bool harqEnabled = true;
  bool fixedTti = false;
  unsigned symPerSf = 24;
  double sfPeriod = 100.0;

  // Command line arguments
  CommandLine cmd;
  cmd.Parse(argc, argv);

  UintegerValue uintegerValue;
  BooleanValue booleanValue;
  StringValue stringValue;
  DoubleValue doubleValue;

  // Variables for the RT 
  int windowForTransient = 150; // number of samples for the vector to use in the filter
  GlobalValue::GetValueByName("reportTablePeriodicity", uintegerValue);
  int ReportTablePeriodicity = (int)uintegerValue.Get(); // in microseconds
  if(ReportTablePeriodicity == 1600)
  {
    windowForTransient = 150;
  }
  else if(ReportTablePeriodicity == 25600)
  {
    windowForTransient = 50;
  }
  else if(ReportTablePeriodicity == 12800)
  {
    windowForTransient = 100;
  }
  else
  {
    NS_ASSERT_MSG(false, "Unrecognized");
  }

  int vectorTransient = windowForTransient*ReportTablePeriodicity;
  GlobalValue::GetValueByName("fastSwitching", booleanValue);
  bool fastSwitching = booleanValue.Get();
  bool hardHandover = !fastSwitching;

  // params for RT, filter, HO mode
  GlobalValue::GetValueByName("noiseAndFilter", booleanValue);
  bool noiseAndFilter = booleanValue.Get();
  GlobalValue::GetValueByName("handoverMode", uintegerValue);
  uint8_t hoMode = uintegerValue.Get();
  GlobalValue::GetValueByName("outageTh", doubleValue);
  double outageTh = doubleValue.Get();

  GlobalValue::GetValueByName("rlcAmEnabled", booleanValue);
  bool rlcAmEnabled = booleanValue.Get();
  GlobalValue::GetValueByName("bufferSize", uintegerValue);
  uint32_t bufferSize = uintegerValue.Get();
  GlobalValue::GetValueByName("interPckInterval", uintegerValue);
  uint32_t interPacketInterval = uintegerValue.Get();
  GlobalValue::GetValueByName("x2Latency", doubleValue);
  double x2Latency = doubleValue.Get();
  GlobalValue::GetValueByName("mmeLatency", doubleValue);
  double mmeLatency = doubleValue.Get();

  double transientDuration = double(vectorTransient)/1000000;
  double simTime = 60;//30.0;//60;//ms 0.005;//630;

  NS_LOG_UNCOND("fastSwitching " << fastSwitching << " rlcAmEnabled " << rlcAmEnabled << " bufferSize " << bufferSize << " interPacketInterval " << 
      interPacketInterval << " x2Latency " << x2Latency << " mmeLatency " << mmeLatency);

  // rng things

  GlobalValue::GetValueByName("runNumber", uintegerValue);
  uint32_t runSet = uintegerValue.Get();
  uint32_t seedSet = 5;
  RngSeedManager::SetSeed (seedSet);
  RngSeedManager::SetRun (runSet); 
  char seedSetStr[21];
  char runSetStr[21];
  sprintf(seedSetStr, "%d", seedSet);
  sprintf(runSetStr, "%d", runSet);

  GlobalValue::GetValueByName("outPath", stringValue);
  std::string path = stringValue.Get();
  std::string mmWaveOutName = "MmWaveSwitchStats";
  std::string lteOutName = "LteSwitchStats";
  std::string dlRlcOutName = "DlRlcStats";
  std::string dlPdcpOutName = "DlPdcpStats";
  std::string ulRlcOutName = "UlRlcStats";
  std::string ulPdcpOutName = "UlPdcpStats";
  std::string  ueHandoverStartOutName =  "UeHandoverStartStats";
  std::string enbHandoverStartOutName = "EnbHandoverStartStats";
  std::string  ueHandoverEndOutName =  "UeHandoverEndStats";
  std::string enbHandoverEndOutName = "EnbHandoverEndStats";
  std::string cellIdInTimeOutName = "CellIdStats";
  std::string cellIdInTimeHandoverOutName = "CellIdStatsHandover";
  std::string mmWaveSinrOutputFilename = "MmWaveSinrTime";
  std::string x2statOutputFilename = "X2Stats";
  std::string udpSentFilename = "UdpSent";
  std::string udpReceivedFilename = "UdpReceived";
  std::string extension = ".txt";
  std::string version;
  if(fastSwitching)
  {
    version = "mc";
    Config::SetDefault ("ns3::MmWaveUeMac::UpdateUeSinrEstimatePeriod", DoubleValue (0));
  }
  else if(hardHandover)
  {
    version = "hh";
  }
  //get current time
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80];
  time (&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(buffer,80,"%d_%m_%Y_%I_%M_%S",timeinfo);
  std::string time_str(buffer);

  Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(rlcAmEnabled));
  Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue(harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::FixedTti", BooleanValue(fixedTti));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::SymPerSlot", UintegerValue(6));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::ResourceBlockNum", UintegerValue(1));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkPerRB", UintegerValue(72));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::SymbolsPerSubframe", UintegerValue(symPerSf));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::SubframePeriod", DoubleValue(sfPeriod));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::TbDecodeLatency", UintegerValue(200.0));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::NumHarqProcess", UintegerValue(100));
  Config::SetDefault ("ns3::MmWaveBeamforming::LongTermUpdatePeriod", TimeValue (MilliSeconds (100.0)));
  Config::SetDefault ("ns3::LteEnbRrc::SystemInformationPeriodicity", TimeValue (MilliSeconds (5.0)));
  Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
  Config::SetDefault ("ns3::LteRlcUmLowLat::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
  Config::SetDefault ("ns3::LteEnbRrc::FirstSibTime", UintegerValue (2));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDelay", TimeValue (MicroSeconds(x2Latency)));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDataRate", DataRateValue(DataRate ("1000Gb/s")));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkMtu",  UintegerValue(10000));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1uLinkDelay", TimeValue (MicroSeconds(1000)));
  Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1apLinkDelay", TimeValue (MicroSeconds(mmeLatency)));
  Config::SetDefault ("ns3::McStatsCalculator::MmWaveOutputFilename", StringValue                 (path + version + mmWaveOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::McStatsCalculator::LteOutputFilename", StringValue                    (path + version + lteOutName    + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::McStatsCalculator::CellIdInTimeOutputFilename", StringValue           (path + version + cellIdInTimeOutName    + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsCalculator::DlRlcOutputFilename", StringValue        (path + version + dlRlcOutName   + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsCalculator::UlRlcOutputFilename", StringValue        (path + version + ulRlcOutName   + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsCalculator::DlPdcpOutputFilename", StringValue       (path + version + dlPdcpOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsCalculator::UlPdcpOutputFilename", StringValue       (path + version + ulPdcpOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::UeHandoverStartOutputFilename", StringValue    (path + version +  ueHandoverStartOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::EnbHandoverStartOutputFilename", StringValue   (path + version + enbHandoverStartOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::UeHandoverEndOutputFilename", StringValue    (path + version +  ueHandoverEndOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::EnbHandoverEndOutputFilename", StringValue   (path + version + enbHandoverEndOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::CellIdStatsHandoverOutputFilename", StringValue(path + version + cellIdInTimeHandoverOutName + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::MmWaveBearerStatsConnector::MmWaveSinrOutputFilename", StringValue(path + version + mmWaveSinrOutputFilename + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::CoreNetworkStatsCalculator::X2FileName", StringValue                  (path + version + x2statOutputFilename    + "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  //  std::string lostFilename = path + version + "LostUdpPackets" +  "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension;
  //  Config::SetDefault ("ns3::UdpServer::ReceivedPacketsFilename", StringValue(path + version + "ReceivedUdp" +  "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  //  Config::SetDefault ("ns3::UdpClient::SentPacketsFilename", StringValue(path + version + "SentUdp" +  "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  //  Config::SetDefault ("ns3::UdpServer::ReceivedSnFilename", StringValue(path + version + "ReceivedSn" +  "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));
  Config::SetDefault ("ns3::LteRlcAm::BufferSizeFilename", StringValue(path + version + "RlcAmBufferSize" +  "_" + seedSetStr + "_" + runSetStr + "_" + time_str + extension));

  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (bufferSize * 1024 * 1024));
  Config::SetDefault ("ns3::LteRlcUmLowLat::MaxTxBufferSize", UintegerValue (bufferSize * 1024 * 1024));
  Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(10.0)));
  Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (bufferSize * 1024 * 1024));

  // handover and RT related params
  switch(hoMode)
  {
    case 1:
        Config::SetDefault ("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue(LteEnbRrc::THRESHOLD));
        break;
    case 2:
        Config::SetDefault ("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue(LteEnbRrc::FIXED_TTT));
        break;
    case 3:
        Config::SetDefault ("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue(LteEnbRrc::DYNAMIC_TTT));
        break;
  }
  
  Config::SetDefault ("ns3::LteEnbRrc::FixedTttValue", UintegerValue (150));
  Config::SetDefault ("ns3::LteEnbRrc::CrtPeriod", IntegerValue (ReportTablePeriodicity));
  Config::SetDefault ("ns3::LteEnbRrc::OutageThreshold", DoubleValue (outageTh));
  Config::SetDefault ("ns3::MmWaveEnbPhy::UpdateSinrEstimatePeriod", IntegerValue (ReportTablePeriodicity));
  Config::SetDefault ("ns3::MmWaveEnbPhy::Transient", IntegerValue (vectorTransient));
  Config::SetDefault ("ns3::MmWaveEnbPhy::NoiseAndFilter", BooleanValue(noiseAndFilter));

  GlobalValue::GetValueByName("lteUplink", booleanValue);
  bool lteUplink = booleanValue.Get();

  Config::SetDefault("ns3::McUePdcp::LteUplink", BooleanValue(lteUplink));
  std::cout << "Lte uplink " << lteUplink << "\n";

  // settings for the 3GPP the channel
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ChannelCondition", StringValue("a"));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Scenario", StringValue("UMi-StreetCanyon"));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::OptionalNlos", BooleanValue(true));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Shadowing", BooleanValue(true)); // enable or disable the shadowing effect
  Config::SetDefault ("ns3::MmWave3gppBuildingsPropagationLossModel::UpdateCondition", BooleanValue(true)); // enable or disable the LOS/NLOS update when the UE moves
  Config::SetDefault ("ns3::AntennaArrayModel::AntennaHorizontalSpacing", DoubleValue(0.5));
  Config::SetDefault ("ns3::AntennaArrayModel::AntennaVerticalSpacing", DoubleValue(0.5));
  Config::SetDefault ("ns3::MmWave3gppChannel::UpdatePeriod", TimeValue(MilliSeconds(100))); // interval after which the channel for a moving user is updated, 
                                                                                       // with spatial consistency procedure. If 0, spatial consistency is not used
  Config::SetDefault ("ns3::MmWave3gppChannel::CellScan", BooleanValue(false)); // Set true to use cell scanning method, false to use the default power method.
  Config::SetDefault ("ns3::MmWave3gppChannel::Blockage", BooleanValue(true)); // use blockage or not
  Config::SetDefault ("ns3::MmWave3gppChannel::PortraitMode", BooleanValue(true)); // use blockage model with UT in portrait mode
  Config::SetDefault ("ns3::MmWave3gppChannel::NumNonselfBlocking", IntegerValue(4)); // number of non-self blocking obstacles

  Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> (); 
  if(true)
  {
    mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::MmWave3gppBuildingsPropagationLossModel"));
  }
  else
  {
    mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::MmWave3gppPropagationLossModel"));
  }
  mmwaveHelper->SetAttribute ("ChannelModel", StringValue ("ns3::MmWave3gppChannel"));


  // create LTE, mmWave eNB nodes and UE node
  NodeContainer ueNodes;
  Ns2MobilityHelper ns2 = Ns2MobilityHelper("/home/luca/Scrivania/TLCnet_prj/crossroad/ns3/traces/mobility.tcl");

  ueNodes.Create(12);
  
  // configure movements for each node
  ns2.Install ();
  
  NodeContainer mmWaveEnbNodes;
  NodeContainer lteEnbNodes;
  NodeContainer allEnbNodes;

  mmWaveEnbNodes.Create(4);
  lteEnbNodes.Create(1);
  
  allEnbNodes.Add(lteEnbNodes);
  allEnbNodes.Add(mmWaveEnbNodes);


  Ptr<MmWavePointToPointEpcHelper> epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
  mmwaveHelper->SetEpcHelper (epcHelper);
  mmwaveHelper->SetHarqEnabled (harqEnabled);
  //    mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::BuildingsObstaclePropagationLossModel"));
  mmwaveHelper->Initialize();

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  // Get SGW/PGW and create a single RemoteHost 
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet by connecting remoteHost to pgw. Setup routing too
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  // Positions
  Vector mmw1Position = Vector(80, 88, 3);
  Vector mmw2Position = Vector(120, 88, 3);
  Vector mmw3Position = Vector(112, 40, 3);
  Vector mmw4Position = Vector(200, 88, 3);

  // Buildings
  Ptr<Building> buildingLeft = CreateObject<Building> ();
  buildingLeft->SetBoundaries (Box (6, 86, 6, 86, 0, 40)); // 14 m from the street
  
  Ptr<Building> buildingRight = CreateObject<Building> ();
  buildingRight->SetBoundaries (Box (114, 194, 6, 86, 0, 40));

  uint32_t treeSize = 3;
  uint32_t treeSpacing = 7;
  uint32_t nTrees = 7;

  for (uint32_t t = 0; t < nTrees; t++)
  {
    Ptr<Building> treeLeft = CreateObject<Building> ();
    treeLeft->SetBoundaries (Box (86 - (t+1)*treeSize - t*treeSpacing, 86 - t*(treeSize + treeSpacing), 90, 90 + treeSize, 0, 6));

    Ptr<Building> treeRight = CreateObject<Building> ();
    treeRight->SetBoundaries (Box (114 + (t+1)*treeSize + t*treeSpacing, 114 + t*(treeSize + treeSpacing), 90, 90 + treeSize, 0, 6));

    Ptr<Building> treeDownLeft = CreateObject<Building> ();
    treeDownLeft->SetBoundaries (Box (90, 90 + treeSize, 86 - (t+1)*treeSize - t*treeSpacing, 86 - t*(treeSize + treeSpacing), 0, 6));

    Ptr<Building> treeDownRight = CreateObject<Building> ();
    treeDownRight->SetBoundaries (Box (110 - treeSize, 110, 86 - (t+1)*treeSize - t*treeSpacing, 86 - t*(treeSize + treeSpacing), 0, 6));
  }

  //Install Mobility Model
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  //enbPositionAlloc->Add (Vector ((double)mmWaveDist/2 + streetWidth, mmw1Dist + 2*streetWidth, mmWaveZ));
  enbPositionAlloc->Add (mmw1Position); // LTE BS, out of area where buildings are deployed
  enbPositionAlloc->Add (mmw1Position);
  enbPositionAlloc->Add (mmw2Position);
  enbPositionAlloc->Add (mmw3Position);
  enbPositionAlloc->Add (mmw4Position);
  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator(enbPositionAlloc);
  enbmobility.Install (allEnbNodes);
  BuildingsHelper::Install (allEnbNodes);

  //    ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (ueInitialPosition, -5, 0));

  BuildingsHelper::Install (ueNodes);

  // Install mmWave, lte, mc Devices to the nodes
  NetDeviceContainer lteEnbDevs = mmwaveHelper->InstallLteEnbDevice (lteEnbNodes);
  NetDeviceContainer mmWaveEnbDevs = mmwaveHelper->InstallEnbDevice (mmWaveEnbNodes);
  NetDeviceContainer mcUeDevs;

  if(fastSwitching)
  {
    mcUeDevs = mmwaveHelper->InstallMcUeDevice (ueNodes);
  } 
  else if(hardHandover)
  {
    mcUeDevs = mmwaveHelper->InstallInterRatHoCapableUeDevice (ueNodes);
  }
  else
  {
    NS_FATAL_ERROR("Invalid option");
  }

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (mcUeDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
    Ptr<Node> ueNode = ueNodes.Get (u);
    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

  // Add X2 interfaces
  mmwaveHelper->AddX2Interface (lteEnbNodes, mmWaveEnbNodes);

  // Manual attachment
  if(fastSwitching)
  {
    mmwaveHelper->AttachToClosestEnb (mcUeDevs, mmWaveEnbDevs, lteEnbDevs);  
  }
  else if(hardHandover)
  {
    mmwaveHelper->AttachIrToClosestEnb (mcUeDevs, mmWaveEnbDevs, lteEnbDevs);
  }


  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  bool dl = 1;
  bool ul = 0;


  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
      if(dl)
      {
        UdpServerHelper dlPacketSinkHelper (dlPort);
        dlPacketSinkHelper.SetAttribute ("PacketWindowSize", UintegerValue(256));
        serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));

        // Simulator::Schedule(MilliSeconds(20), &PrintLostUdpPackets, DynamicCast<UdpServer>(serverApps.Get(serverApps.GetN()-1)), lostFilename);

        UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
        dlClient.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
        dlClient.SetAttribute ("MaxPackets", UintegerValue(0xFFFFFFFF));
        clientApps.Add (dlClient.Install (remoteHost));

      }
      if(ul)
      {
        ++ulPort;
        PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
        ulPacketSinkHelper.SetAttribute ("PacketWindowSize", UintegerValue(256));
        serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
        UdpClientHelper ulClient (remoteHostAddr, ulPort);
        ulClient.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
        ulClient.SetAttribute ("MaxPackets", UintegerValue(0xFFFFFFFF));
        clientApps.Add (ulClient.Install (ueNodes.Get(u)));
      }
  }

  // Start applications.
  NS_LOG_UNCOND("transientDuration " << transientDuration << " simTime " << simTime);
  serverApps.Start (Seconds(transientDuration));
  clientApps.Start (Seconds(transientDuration));
  clientApps.Stop (Seconds(simTime - 1));

  // Simulator::Schedule(Seconds(transientDuration), &ChangeSpeed, ueNodes.Get(0), Vector(ueSpeed, 0, 0)); // start UE movement after Seconds(0.5)
  // Simulator::Schedule(Seconds(simTime - 1), &ChangeSpeed, ueNodes.Get(0), Vector(0, 0, 0)); // start UE movement after Seconds(0.5)
  
  double numPrints = 0;
  for(int i = 0; i < numPrints; i++)
  {
   Simulator::Schedule(Seconds(i*simTime/numPrints), &PrintPosition, ueNodes.Get(0)); 
  }

  BuildingsHelper::MakeMobilityModelConsistent ();

//  Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
//  lteHelper->EnablePdcpTraces();
//  lteHelper->EnableRlcTraces();

  mmwaveHelper->EnableTraces ();

  // set to true if you want to print the map of buildings, ues and enbs
  bool print = false;
  if(print)
  {
    PrintGnuplottableBuildingListToFile("buildings.txt");
    PrintGnuplottableEnbListToFile("enbs.txt");
    PrintGnuplottableUeListToFile("ues.txt");
  }
  else
  {
    AnimationInterface anim ("animation.xml");
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();    
  }
  
  Simulator::Destroy();
  return 0;
}
