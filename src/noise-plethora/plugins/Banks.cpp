#include "Banks.hpp"
#include "Banks_Def.hpp"

const Bank::BankElem Bank::defaultElem = {"", 1.0};

Bank::Bank() {
	programs.fill(defaultElem);
}

Bank::Bank(const BankElem& p1, const BankElem& p2, const BankElem& p3,
           const BankElem& p4, const BankElem& p5, const BankElem& p6,
           const BankElem& p7, const BankElem& p8, const BankElem& p9,
           const BankElem& p10)
	: programs{p1, p2, p3, p4, p5, p6, p7, p8, p9, p10}
{ }

const std::string Bank::getProgramName(int i) {
	if (i >= 0 && i < programsPerBank) {
		return programs[i].name;
	}
	return "";
}

float Bank::getProgramGain(int i) {
	if (i >= 0 && i < programsPerBank) {
		return programs[i].gain;
	}
	return 1.0;
}

int Bank::getSize() {
	int size = 0;
	for (auto it = programs.begin(); it != programs.end(); it++) {
		if ((*it).name == "") {
			break;
		}
		size++;
	}
	return size;
}



// Bank A:
#include "P_radioOhNo.hpp"
#include "P_Rwalk_SineFMFlange.hpp"
#include "P_xModRingSqr.hpp"
#include "P_XModRingSine.hpp"
#include "P_CrossModRing.hpp"
#include "P_resonoise.hpp"
#include "P_grainGlitch.hpp"
#include "P_grainGlitchII.hpp"
#include "P_grainGlitchIII.hpp"
#include "P_basurilla.hpp"

// Bank B: HH clusters
#include "P_clusterSaw.hpp"
#include "P_pwCluster.hpp"
#include "P_crCluster2.hpp"
#include "P_sineFMcluster.hpp"
#include "P_TriFMcluster.hpp"
#include "P_PrimeCluster.hpp"
#include "P_PrimeCnoise.hpp"
#include "P_FibonacciCluster.hpp"
#include "P_partialCluster.hpp"
#include "P_phasingCluster.hpp"

// Bank C: harsh and wild
#include "P_BasuraTotal.hpp"
#include "P_Atari.hpp"
#include "P_WalkingFilomena.hpp"
#include "P_S_H.hpp"
#include "P_arrayOnTheRocks.hpp"
#include "P_existencelsPain.hpp"
#include "P_whoKnows.hpp"
#include "P_satanWorkout.hpp"
#include "P_Rwalk_BitCrushPW.hpp"
#include "P_Rwalk_LFree.hpp"

// Bank D: Test / other
//#include "P_TestPlugin.hpp"
//#include "P_TeensyAlt.hpp"
//#include "P_WhiteNoise.hpp"
//#include "P_Rwalk_LBit.hpp"
//#include "P_Rwalk_SineFM.hpp"
//#include "P_VarWave.hpp"
//#include "P_RwalkVarWave.hpp"
//#include "P_Rwalk_ModWave.hpp"
//#include "P_Rwalk_WaveTwist.hpp"


static const Bank bank1 BANKS_DEF_1; // Banks_Def.hpp
static const Bank bank2 BANKS_DEF_2;
static const Bank bank3 BANKS_DEF_3;
//static const Bank bank4 BANKS_DEF_4;
//static const Bank bank5 BANKS_DEF_5;
static std::array<Bank, numBanks> banks { bank1, bank2, bank3 }; //, bank5 };

// static const Bank bank6 BANKS_DEF_6;
// static const Bank bank7 BANKS_DEF_7;
// static const Bank bank8 BANKS_DEF_8;
// static const Bank bank9 BANKS_DEF_9;
// static const Bank bank10 BANKS_DEF_10;
// static std::array<Bank, programsPerBank> banks { bank1, bank2, bank3, bank4, bank5, bank6, bank7, bank8, bank9, bank10 };

Bank& getBankForIndex(int i) {
	if (i < 0)
		i = 0;
	if (i >= programsPerBank)
		i = (programsPerBank - 1);
	return banks[i];
}
