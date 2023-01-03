#include "TowerInfoContainerv1.h"
#include "TowerInfov1.h"

#include <phool/PHObject.h>

#include <TClonesArray.h>

int emcadc[8][8] = {
   {62,60,46,44,30,28,14,12},
   {63,61,47,45,31,29,15,13},
   {58,56,42,40,26,24,10,8},
   {59,57,43,41,27,25,11,9},
   {54,52,38,36,22,20,6,4},
   {55,53,39,37,23,21,7,5},
   {50,48,34,32,18,16,2,0},
   {51,49,35,33,19,17,3,1}
};

int hcaladc[8][2] = {
   {0,1},
   {2,3},
   {4,5},
   {6,7},
   {8,9},
   {10,11},
   {12,13},
   {14,15}
};

TowerInfoContainerv1::TowerInfoContainerv1(DETECTOR detec)
 : _detector(detec)
{
  _clones = new TClonesArray("TowerInfov1", 50);
  _clones->SetOwner();
  _clones->SetName("TowerInfoContainerv1");
}

TowerInfoContainerv1::~TowerInfoContainerv1()
{
  delete _clones;
}

void TowerInfoContainerv1::Reset()
{
  _clones->Clear();
}

void TowerInfoContainerv1::add(TowerInfov1 *ti, int pos)
{
  new ((*_clones)[pos]) TowerInfov1(*ti);
}

TowerInfov1* TowerInfoContainerv1::at(int pos)
{
  return (TowerInfov1*)_clones->At(pos);
}

int TowerInfoContainerv1::encode_key(int towerIndex)
{
  int phimap[64] = {0};
  int etamap[64] = {0};

  int channels_per_sector = -1;
  int supersector = -1;
  int nchannelsperpacket = -1;
  int maxphibin = -1;
  int maxetabin = -1;
  int etabinoffset[4] = {0};
  int phibinoffset[4] = {0};

  if(_detector == DETECTOR::SEPD)
  {
    channels_per_sector = 31;
    supersector = channels_per_sector*12;
    int supersectornumber = towerIndex/supersector;
    int sector = ( (towerIndex%supersector))/channels_per_sector;
    int channel = ( (towerIndex%supersector))%channels_per_sector;
    int rmap[31]={0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15};
    int phimap[31] = {0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1};
    int globalphi = phimap[channel] + 2*sector;
    int r = rmap[channel];
    int key = globalphi+ (r << 10) + (supersectornumber << 20);
    return key;
  }

  if(_detector == DETECTOR::EMCAL)
  {
    for (int j = 0;j < 8;j++)
  	{
  	  for (int k = 0;k < 8;k++)
   	  {
        etamap[emcadc[j][k]] = j;
        phimap[emcadc[j][k]] = k;
   	  }
   	}

    channels_per_sector = 64;
    supersector = 64*12;
    nchannelsperpacket = 64*3;
    maxphibin = 7;
    maxetabin = 23;
    etabinoffset[0] = 24;
    etabinoffset[1] = 0;
    etabinoffset[2]  = 48;
    etabinoffset[3] = 72;
    phibinoffset[0] = 0;
    phibinoffset[1] = 0;
    phibinoffset[2] = 0;
    phibinoffset[3] = 0;
  }
  else if(_detector == DETECTOR::HCAL)
  {
    for (int j = 0; j < 8;j++)
    {
      for (int k = 0;k < 2;k++)
      {
        etamap[hcaladc[j][k]] = j;
        phimap[hcaladc[j][k]] = k;
      }
    }

    channels_per_sector = 16;
    supersector = 16*4*3;
    nchannelsperpacket = channels_per_sector*4;
    maxphibin = 7;
    maxetabin = 23;
    etabinoffset[0] = 0;
    etabinoffset[1] = 8;
    etabinoffset[2] = 16;
    etabinoffset[3] = 0;
    phibinoffset[0] = 0;
    phibinoffset[1] = 2;
    phibinoffset[2] = 4;
    phibinoffset[3] = 6;
  }

  int supersectornumber = towerIndex/supersector;
  int packet = (towerIndex%supersector)/nchannelsperpacket; // 0 = S small |eta|, 1 == S big |eta|, 2 == N small |eta|, 3 == N big |eta|
  int interfaceboard = ( (towerIndex%supersector)%nchannelsperpacket)/channels_per_sector;
  int interfaceboard_channel = ( (towerIndex%supersector)%nchannelsperpacket) % channels_per_sector;

  int localphibin = phimap[interfaceboard_channel] + phibinoffset[interfaceboard];
  if((_detector == DETECTOR::EMCAL) && (packet == 0 || packet == 1))
  {
    localphibin = maxphibin - localphibin;
  }

  int localetabin = etamap[interfaceboard_channel];
  int packet_etabin = 0;

  if(_detector == DETECTOR::EMCAL)
  {
    int packet_etabin = localetabin+8*interfaceboard;
    if(packet == 0 || packet == 1)
    {
      packet_etabin = maxetabin - packet_etabin;
    }
  }
  else if(_detector == DETECTOR::HCAL)
  {
    packet_etabin = localetabin;
  }

  int globaletabin = packet_etabin + etabinoffset[packet];
  int globalphibin = localphibin + supersectornumber*8;

  int key = globalphibin + (globaletabin << 16);

  return key;
}

TowerInfoContainer::TowerMap TowerInfoContainerv1::getTowerMap()
{
  TowerInfoContainer::TowerMap tmap;
  for(unsigned int i = 0; i < size(); i++)
  {
    tmap.insert(std::make_pair(encode_key(i), at(i)));
  }

  return tmap;
}

int TowerInfoContainerv1::getTowerPhiBin(int key)
{
  int etabin = key >> 16;
  int phibin = key - (etabin << 16);
  return phibin;
}

int TowerInfoContainerv1::getTowerEtaBin(int key)
{
  int etabin = key >> 16;
  return etabin;
}
