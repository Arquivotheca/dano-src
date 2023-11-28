# makefile

SRC_DIR := src/addons/print_server/postscript

TARGET_NAME := PostScript

SRCS := 	\
		parser_interface.cpp \
		Convert.cpp	\
		PS_Print.cpp	\
		PS_Setup.cpp	\
		Postscript.cpp	\
		Status.cpp	\
		compress.cpp	\
		ppd_control.cpp	\
		Element.cpp	\
		ppd_scanner.l	\
		ppd_parser.y	\
		PSGenerator.cpp	\
		FontHandler.cpp \
		ttf2pt1.cpp		\
		t1asm.cpp		\
		Scanner.cpp

MY_DEPS := 

MY_CFLAGS := 
MY_INCLUDES := \
	-I$(call FULL_SRC_DIR, $(SRC_DIR)) \
	-I$(BUILDHOME)/src/inc/printclient_p\
	-I$(BUILDHOME)/src/inc/app_server_p\
	-I$(BUILDHOME)/src/inc \
	-I$(call FULL_OBJ_DIR, $(SRC_DIR)) \
#

SYSTEM_LIBS := libroot.so libbe.so \
	libtextencoding.so
PARENT_IMAGE :=

MY_INSTALL_DIR := $(INSTALL_ADD_ONS)/Print

include $(SDIR_TEMPLATES)/AddonTemplate.mk

DEPEND_ON_ME := ppd_parser.h
SRC_DIR := src/addons/print_server/postscript
SRCS := $(SRCS_IN)
include $(SDIR_TEMPLATES)/NoCircleDepTemplate.mk

ZIP_FILE := $(SRC_DIR_IN)/ppd.zip
FILES := \
	ppd/ \
	ppd/Agfa/ \
	ppd/Agfa/ac500503.ppd \
	ppd/Agfa/agac100a.ppd \
	ppd/Agfa/agac100x.ppd \
	ppd/Agfa/agac10wx.ppd \
	ppd/Agfa/agac150a.ppd \
	ppd/Agfa/agac150x.ppd \
	ppd/Agfa/agac15wa.ppd \
	ppd/Agfa/agac15wx.ppd \
	ppd/Agfa/agac800a.ppd \
	ppd/Agfa/agac800x.ppd \
	ppd/Agfa/agaccsf1.ppd \
	ppd/Agfa/agaccst1.ppd \
	ppd/Agfa/agatlasa.ppd \
	ppd/Agfa/agav020a.ppd \
	ppd/Agfa/agav020x.ppd \
	ppd/Agfa/agav025a.ppd \
	ppd/Agfa/agav025x.ppd \
	ppd/Agfa/agav030a.ppd \
	ppd/Agfa/agav030x.ppd \
	ppd/Agfa/agav036a.ppd \
	ppd/Agfa/agav036x.ppd \
	ppd/Agfa/agav044a.ppd \
	ppd/Agfa/agav044x.ppd \
	ppd/Agfa/agav25ea.ppd \
	ppd/Agfa/agav25ex.ppd \
	ppd/Agfa/agav25xa.ppd \
	ppd/Agfa/agav25xx.ppd \
	ppd/Agfa/agduop_1.ppd \
	ppd/Agfa/agduopra.ppd \
	ppd/Agfa/agel5sf1.ppd \
	ppd/Agfa/agfideva.ppd \
	ppd/Agfa/agfidevx.ppd \
	ppd/Agfa/agp94sf1.ppd \
	ppd/Agfa/agp95sf1.ppd \
	ppd/Agfa/agp96sf1.ppd \
	ppd/Agfa/agp97sf1.ppd \
	ppd/Agfa/agp98sf1.ppd \
	ppd/Agfa/agpo100a.ppd \
	ppd/Agfa/agpo100x.ppd \
	ppd/Agfa/agpr9551.ppd \
	ppd/Agfa/agpro941.ppd \
	ppd/Agfa/agpro971.ppd \
	ppd/Agfa/agpro981.ppd \
	ppd/Agfa/agprs961.ppd \
	ppd/Agfa/agsels51.ppd \
	ppd/Agfa/agsels71.ppd \
	ppd/Agfa/agsl7sf1.ppd \
	ppd/Agfa/agss500a.ppd \
	ppd/Agfa/agss500x.ppd \
	ppd/Agfa/agss700a.ppd \
	ppd/Agfa/agss700x.ppd \
	ppd/Agfa/agstds21.ppd \
	ppd/Agfa/amchr518.ppd \
	ppd/Agfa/cg94_493.ppd \
	ppd/Agfa/efaa2010.ppd \
	ppd/Agfa/iptr1a25.ppd \
	ppd/Agfa/iptr1a44.ppd \
	ppd/Agfa/iptr1acc.ppd \
	ppd/Agfa/iptr1l30.ppd \
	ppd/Agfa/iptr1l33.ppd \
	ppd/Agfa/iptr1l50.ppd \
	ppd/Agfa/iptr1l53.ppd \
	ppd/Agfa/iptr1ss5.ppd \
	ppd/Agfa/iptr1ss7.ppd \
	ppd/Apple/ \
	ppd/Apple/a_pnt518.ppd \
	ppd/Apple/ap_ntxj1.ppd \
	ppd/Apple/aplw3101.ppd \
	ppd/Apple/aplw3201.ppd \
	ppd/Apple/aplw3601.ppd \
	ppd/Apple/aplw6101.ppd \
	ppd/Apple/aplw8101.ppd \
	ppd/Apple/aplw81f1.ppd \
	ppd/Apple/aplwbgr1.ppd \
	ppd/Apple/aplwcob1.ppd \
	ppd/Apple/aplwcsb1.ppd \
	ppd/Apple/aplwgrf1.ppd \
	ppd/Apple/aplwgri1.ppd \
	ppd/Apple/aplwgrj1.ppd \
	ppd/Apple/aplwiif1.ppd \
	ppd/Apple/aplwiif2.ppd \
	ppd/Apple/aplwiig1.ppd \
	ppd/Apple/aplwiig2.ppd \
	ppd/Apple/aplwlot1.ppd \
	ppd/Apple/aplwntr1.ppd \
	ppd/Apple/apntx501.ppd \
	ppd/Apple/apple230.ppd \
	ppd/Apple/apple380.ppd \
	ppd/Apple/apple422.ppd \
	ppd/Apple/aptolld1.ppd \
	ppd/Apple/aptollw1.ppd \
	ppd/Apple/lwnt_470.ppd \
	ppd/Apple/lwntx470.ppd \
	ppd/Apple/lwntx518.ppd \
	ppd/Canon/ \
	ppd/Canon/bgpe40w1.ppd \
	ppd/Canon/bgpe60w1.ppd \
	ppd/Canon/bgpe80w1.ppd \
	ppd/Canon/cn126pe1.ppd \
	ppd/Canon/cn2460e1.ppd \
	ppd/Canon/cn2460e2.ppd \
	ppd/Canon/cn7302_1.ppd \
	ppd/Canon/cn7305_1.ppd \
	ppd/Canon/cn_12601.ppd \
	ppd/Canon/cn_500_1.ppd \
	ppd/Canon/cn_500j1.ppd \
	ppd/Canon/cn_a4041.ppd \
	ppd/Canon/cn_b4061.ppd \
	ppd/Canon/cn_psgp1.ppd \
	ppd/Canon/cna40451.ppd \
	ppd/Canon/cnb40651.ppd \
	ppd/Canon/cnlb1261.ppd \
	ppd/Canon/cnlb8601.ppd \
	ppd/Canon/cnlbp4_1.ppd \
	ppd/Canon/cnlbp8_1.ppd \
	ppd/Canon/cnlbp8r1.ppd \
	ppd/Canon/cnlbp8t1.ppd \
	ppd/Canon/cnmdck01.ppd \
	ppd/Canon/cnmdcp01.ppd \
	ppd/Canon/cnps3r31.ppd \
	ppd/Canon/cnps3r51.ppd \
	ppd/Canon/cnps3r71.ppd \
	ppd/Canon/cnps3r81.ppd \
	ppd/Canon/cnpsgpj1.ppd \
	ppd/Canon/efac2b10.ppd \
	ppd/Canon/efac3a10.ppd \
	ppd/Canon/efxjc1a3.ppd \
	ppd/Canon/efxjc1b3.ppd \
	ppd/Canon/efxjc2a3.ppd \
	ppd/Canon/efxjc2b3.ppd \
	ppd/Canon/efxjc3a4.ppd \
	ppd/Canon/efxjc3b4.ppd \
	ppd/Compaq/ \
	ppd/Compaq/cppmq151.ppd \
	ppd/Compaq/cppmq152.ppd \
	ppd/Compaq/cppmq201.ppd \
	ppd/Compaq/cppmq202.ppd \
	ppd/Epson/ \
	ppd/Epson/bgpe15w1.ppd \
	ppd/Epson/bgpe20w1.ppd \
	ppd/Epson/bgpe50w1.ppd \
	ppd/Epson/bgpe52w1.ppd \
	ppd/Epson/bgpeiiw1.ppd \
	ppd/Epson/bgpeiiw2.ppd \
	ppd/Epson/bgpescw1.ppd \
	ppd/Epson/bgpespw1.ppd \
	ppd/Epson/bgpesxw1.ppd \
	ppd/Epson/bgpexpw1.ppd \
	ppd/Epson/ep5000c1.ppd \
	ppd/Epson/ep700v21.ppd \
	ppd/Epson/ep826051.ppd \
	ppd/Epson/ep_08001.ppd \
	ppd/Epson/ep_15201.ppd \
	ppd/Epson/ep_30001.ppd \
	ppd/Epson/ep_50001.ppd \
	ppd/Epson/ep_82f21.ppd \
	ppd/Epson/ep_82f51.ppd \
	ppd/Epson/ep_90001.ppd \
	ppd/Epson/ep_900c1.ppd \
	ppd/Epson/ep_p0701.ppd \
	ppd/Epson/ep_p1201.ppd \
	ppd/Epson/ep_stph1.ppd \
	ppd/Epson/epl3kf21.ppd \
	ppd/Epson/epl3kf51.ppd \
	ppd/Epson/epl75523.ppd \
	ppd/Hewlett\ Packard/ \
	ppd/Hewlett\ Packard/bghp30w1.ppd \
	ppd/Hewlett\ Packard/bghp40w1.ppd \
	ppd/Hewlett\ Packard/bghp50w1.ppd \
	ppd/Hewlett\ Packard/hp1200c1.ppd \
	ppd/Hewlett\ Packard/hp2500c1.ppd \
	ppd/Hewlett\ Packard/hp2500c2.ppd \
	ppd/Hewlett\ Packard/hp3500c1.ppd \
	ppd/Hewlett\ Packard/hp3si523.ppd \
	ppd/Hewlett\ Packard/hp4m3_v1.ppd \
	ppd/Hewlett\ Packard/hp4m6_v1.ppd \
	ppd/Hewlett\ Packard/hp4ml_v1.ppd \
	ppd/Hewlett\ Packard/hp4mp3_1.ppd \
	ppd/Hewlett\ Packard/hp4mp6_1.ppd \
	ppd/Hewlett\ Packard/hp4plus3.ppd \
	ppd/Hewlett\ Packard/hp4plus6.ppd \
	ppd/Hewlett\ Packard/hp4si3_1.ppd \
	ppd/Hewlett\ Packard/hp4si6_1.ppd \
	ppd/Hewlett\ Packard/hp755cm1.ppd \
	ppd/Hewlett\ Packard/hp_3d522.ppd \
	ppd/Hewlett\ Packard/hp_3p522.ppd \
	ppd/Hewlett\ Packard/hp_650c1.ppd \
	ppd/Hewlett\ Packard/hp_650c2.ppd \
	ppd/Hewlett\ Packard/hp_750c1.ppd \
	ppd/Hewlett\ Packard/hp__clj1.ppd \
	ppd/Hewlett\ Packard/hpcopyj1.ppd \
	ppd/Hewlett\ Packard/hpiid522.ppd \
	ppd/Hewlett\ Packard/hpiii522.ppd \
	ppd/Hewlett\ Packard/hpiip522.ppd \
	ppd/Hewlett\ Packard/hplj4mv1.ppd \
	ppd/Hewlett\ Packard/hplj5m_4.ppd \
	ppd/Hewlett\ Packard/hplj5p_1.ppd \
	ppd/Hewlett\ Packard/hplj5si1.ppd \
	ppd/Hewlett\ Packard/hplj5sim.ppd \
	ppd/Hewlett\ Packard/hplj_3d1.ppd \
	ppd/Hewlett\ Packard/hplj_3p1.ppd \
	ppd/Hewlett\ Packard/hplj__31.ppd \
	ppd/Hewlett\ Packard/hppjxl31.ppd \
	ppd/IBM/ \
	ppd/IBM/ib4029a1.ppd \
	ppd/IBM/ib4029b1.ppd \
	ppd/IBM/ibm12wj1.ppd \
	ppd/IBM/ibm17523.ppd \
	ppd/IBM/ibm17wj1.ppd \
	ppd/IBM/ibm20470.ppd \
	ppd/IBM/ibm24wj1.ppd \
	ppd/IBM/ibm30505.ppd \
	ppd/IBM/ibm39523.ppd \
	ppd/IBM/ibm43032.ppd \
	ppd/IBM/ibm43121.ppd \
	ppd/IBM/ibm43171.ppd \
	ppd/IBM/ibm43201.ppd \
	ppd/IBM/ibm43241.ppd \
	ppd/IBM/ibmifp4k.ppd \
	ppd/IBM/ibmifp60.ppd \
	ppd/IBM/ibmpod11.ppd \
	ppd/Kodak/ \
	ppd/Kodak/efxjk3f3.ppd \
	ppd/Kodak/kd8400p1.ppd \
	ppd/Kodak/kd8600j1.ppd \
	ppd/Kodak/kd8600p1.ppd \
	ppd/Kodak/kd8600p2.ppd \
	ppd/Kodak/kd8650p1.ppd \
	ppd/Kodak/kd92p001.ppd \
	ppd/Kodak/kd_15801.ppd \
	ppd/Kodak/kdh00001.ppd \
	ppd/Kodak/kdh00002.ppd \
	ppd/Kodak/kdis92p1.ppd \
	ppd/Linotype/ \
	ppd/Linotype/l100_425.ppd \
	ppd/Linotype/l200_471.ppd \
	ppd/Linotype/l200_493.ppd \
	ppd/Linotype/l300_471.ppd \
	ppd/Linotype/l300_493.ppd \
	ppd/Linotype/l500_493.ppd \
	ppd/Linotype/lh230__1.ppd \
	ppd/Linotype/lh260__1.ppd \
	ppd/Linotype/lh300__1.ppd \
	ppd/Linotype/lh3020i3.ppd \
	ppd/Linotype/lh3020j4.ppd \
	ppd/Linotype/lh3030i3.ppd \
	ppd/Linotype/lh3030j4.ppd \
	ppd/Linotype/lh330__1.ppd \
	ppd/Linotype/lh500__1.ppd \
	ppd/Linotype/lh530__1.ppd \
	ppd/Linotype/lh630__1.ppd \
	ppd/Linotype/lh830__1.ppd \
	ppd/Linotype/lh930__1.ppd \
	ppd/Linotype/lh_260_3.ppd \
	ppd/Linotype/lh_260j4.ppd \
	ppd/Linotype/lh_330_3.ppd \
	ppd/Linotype/lh_330j4.ppd \
	ppd/Linotype/lh_560_3.ppd \
	ppd/Linotype/lh_560j4.ppd \
	ppd/Linotype/lh_630_3.ppd \
	ppd/Linotype/lh_630j4.ppd \
	ppd/Linotype/lh_830_4.ppd \
	ppd/Linotype/lh_830j4.ppd \
	ppd/Linotype/lh_930_4.ppd \
	ppd/Linotype/lh_930j4.ppd \
	ppd/Linotype/lhcoflc5.ppd \
	ppd/Linotype/lhcoflr5.ppd \
	ppd/Linotype/lhgubeh3.ppd \
	ppd/Linotype/lhgubhj4.ppd \
	ppd/Linotype/lhherhj4.ppd \
	ppd/Linotype/lhherij4.ppd \
	ppd/Linotype/lhherkh3.ppd \
	ppd/Linotype/lhherki4.ppd \
	ppd/Linotype/lhhrkph7.ppd \
	ppd/Linotype/lhhrkpi7.ppd \
	ppd/Linotype/lhpr60_1.ppd \
	ppd/Linotype/lhqushj4.ppd \
	ppd/Linotype/lhqusij4.ppd \
	ppd/Linotype/lhqusrh3.ppd \
	ppd/Linotype/lhqusri3.ppd \
	ppd/Linotype/lhsighj4.ppd \
	ppd/Linotype/lhsigij4.ppd \
	ppd/Linotype/lhsignh3.ppd \
	ppd/Linotype/lhsigni3.ppd \
	ppd/OKI/ \
	ppd/OKI/defprtr2.ppd \
	ppd/OKI/ok1200_1.ppd \
	ppd/OKI/ok1200a1.ppd \
	ppd/OKI/ok1200l1.ppd \
	ppd/OKI/ok1200x1.ppd \
	ppd/OKI/ok400ii1.ppd \
	ppd/OKI/ok7002_1.ppd \
	ppd/OKI/ok7032_1.ppd \
	ppd/OKI/ok800lt1.ppd \
	ppd/OKI/ok801f51.ppd \
	ppd/OKI/ok801pf1.ppd \
	ppd/OKI/ok9003_1.ppd \
	ppd/OKI/ok9023_1.ppd \
	ppd/OKI/ok9033_1.ppd \
	ppd/OKI/ok9033f1.ppd \
	ppd/OKI/ok9053f1.ppd \
	ppd/OKI/ok_400e1.ppd \
	ppd/OKI/ok_410e1.ppd \
	ppd/OKI/okg68kj2.ppd \
	ppd/OKI/okg68kj5.ppd \
	ppd/OKI/okm90021.ppd \
	ppd/OKI/okm90221.ppd \
	ppd/OKI/okm90251.ppd \
	ppd/OKI/okm90321.ppd \
	ppd/OKI/okm90351.ppd \
	ppd/OKI/okm90521.ppd \
	ppd/OKI/okm90551.ppd \
	ppd/OKI/okml703n.ppd \
	ppd/OKI/okml8011.ppd \
	ppd/OKI/okol8501.ppd \
	ppd/OKI/okol8701.ppd \
	ppd/OKI/okop10_1.ppd \
	ppd/OKI/okop10x1.ppd \
	ppd/OKI/okop12b1.ppd \
	ppd/OKI/okp804_1.ppd \
	ppd/OKI/ol830525.ppd \
	ppd/OKI/ol840518.ppd \
	ppd/Tektronix/ \
	ppd/Tektronix/tk200171.ppd \
	ppd/Tektronix/tk200172.ppd \
	ppd/Tektronix/tk200391.ppd \
	ppd/Tektronix/tk200392.ppd \
	ppd/Tektronix/tk220171.ppd \
	ppd/Tektronix/tk220391.ppd \
	ppd/Tektronix/tk240171.ppd \
	ppd/Tektronix/tk240172.ppd \
	ppd/Tektronix/tk240391.ppd \
	ppd/Tektronix/tk240392.ppd \
	ppd/Tektronix/tk300pj1.ppd \
	ppd/Tektronix/tk300xj1.ppd \
	ppd/Tektronix/tk300xp1.ppd \
	ppd/Tektronix/tk340171.ppd \
	ppd/Tektronix/tk340172.ppd \
	ppd/Tektronix/tk340691.ppd \
	ppd/Tektronix/tk340692.ppd \
	ppd/Tektronix/tk340pj2.ppd \
	ppd/Tektronix/tk350171.ppd \
	ppd/Tektronix/tk350172.ppd \
	ppd/Tektronix/tk350691.ppd \
	ppd/Tektronix/tk350692.ppd \
	ppd/Tektronix/tk350pj2.ppd \
	ppd/Tektronix/tk360j21.ppd \
	ppd/Tektronix/tk360j51.ppd \
	ppd/Tektronix/tk360j71.ppd \
	ppd/Tektronix/tk450pj1.ppd \
	ppd/Tektronix/tk480xj1.ppd \
	ppd/Tektronix/tk550171.ppd \
	ppd/Tektronix/tk550391.ppd \
	ppd/Tektronix/tk550pj1.ppd \
	ppd/Tektronix/tkp140j1.ppd \
	ppd/Tektronix/tkp200i1.ppd \
	ppd/Tektronix/tkp200i2.ppd \
	ppd/Tektronix/tkp200j1.ppd \
	ppd/Tektronix/tkp220i1.ppd \
	ppd/Tektronix/tkp240j1.ppd \
	ppd/Tektronix/tkp300i1.ppd \
	ppd/Tektronix/tkp300j1.ppd \
	ppd/Tektronix/tkp300x1.ppd \
	ppd/Tektronix/tkp340j1.ppd \
	ppd/Tektronix/tkp340j2.ppd \
	ppd/Tektronix/tkp350j2.ppd \
	ppd/Tektronix/tkp360p1.ppd \
	ppd/Tektronix/tkp380j1.ppd \
	ppd/Tektronix/tkp380p1.ppd \
	ppd/Tektronix/tkp440j1.ppd \
	ppd/Tektronix/tkp450j1.ppd \
	ppd/Tektronix/tkp450p1.ppd \
	ppd/Tektronix/tkp480j1.ppd \
	ppd/Tektronix/tkp480x1.ppd \
	ppd/Tektronix/tkp540j1.ppd \
	ppd/Tektronix/tkp540p1.ppd \
	ppd/Tektronix/tkp550j1.ppd \
	ppd/Tektronix/tkp560j1.ppd \
	ppd/Tektronix/tkp560p1.ppd \
	ppd/Tektronix/tkp600j1.ppd \
	ppd/Tektronix/tkph1401.ppd \
	ppd/Tektronix/tkph2sd1.ppd \
	ppd/Tektronix/tkph3801.ppd \
	ppd/Tektronix/tkph4401.ppd \
	ppd/Tektronix/tkph4501.ppd \
	ppd/Tektronix/tkph4801.ppd \
	ppd/Tektronix/tkph5401.ppd \
	ppd/Tektronix/tkph5601.ppd \
	ppd/Tektronix/tkph6001.ppd \
	ppd/Tektronix/tkphpxi1.ppd \
	ppd/Tektronix/tkphz2j1.ppd \
	ppd/Tektronix/tkphz3j1.ppd \
	ppd/Tektronix/tkphzr21.ppd \
	ppd/Tektronix/tkphzr22.ppd \
	ppd/Tektronix/tkphzr31.ppd \
	ppd/Tektronix/tkphzr32.ppd \
	ppd/Tektronix/tkpxe171.ppd \
	ppd/Tektronix/tkpxe391.ppd \
	ppd/Texas\ Instrument/ \
	ppd/Texas\ Instrument/t1513470.ppd \
	ppd/Texas\ Instrument/t1535470.ppd \
	ppd/Texas\ Instrument/ti08_450.ppd \
	ppd/Texas\ Instrument/til23521.ppd \
	ppd/Texas\ Instrument/til65521.ppd \
	ppd/Texas\ Instrument/tim12641.ppd \
	ppd/Texas\ Instrument/tim17521.ppd \
	ppd/Texas\ Instrument/tim35521.ppd \
	ppd/Texas\ Instrument/tim86411.ppd \
	ppd/Texas\ Instrument/timl6231.ppd \
	ppd/Texas\ Instrument/timlp231.ppd \
	ppd/Texas\ Instrument/timlp232.ppd \
	ppd/Texas\ Instrument/timlp233.ppd \
	ppd/Texas\ Instrument/timlp651.ppd \
	ppd/Texas\ Instrument/timlp652.ppd \
	ppd/Texas\ Instrument/timlp671.ppd \
	ppd/Texas\ Instrument/timlp672.ppd \
	ppd/Texas\ Instrument/timp3231.ppd \
	ppd/Texas\ Instrument/timp3232.ppd \
	ppd/Texas\ Instrument/timpe231.ppd \
	ppd/Texas\ Instrument/titrb161.ppd \
	ppd/Texas\ Instrument/titrbo61.ppd \
	ppd/Texas\ Instrument/titrbo91.ppd \
	ppd/Texas\ Instrument/tix17521.ppd \
	ppd/Texas\ Instrument/tix35521.ppd \
	ppd/Xerox/ \
	ppd/Xerox/efax3010.ppd \
	ppd/Xerox/efax4010.ppd \
	ppd/Xerox/efax5010.ppd \
	ppd/Xerox/efax7010.ppd \
	ppd/Xerox/efmx3010.ppd \
	ppd/Xerox/efmx5010.ppd \
	ppd/Xerox/efmx7010.ppd \
	ppd/Xerox/efsix203.ppd \
	ppd/Xerox/efsix303.ppd \
	ppd/Xerox/efsix503.ppd \
	ppd/Xerox/efsix703.ppd \
	ppd/Xerox/smmjstk2.ppd \
	ppd/Xerox/xr049152.ppd \
	ppd/Xerox/xr049154.ppd \
	ppd/Xerox/xr049202.ppd \
	ppd/Xerox/xr_42151.ppd \
	ppd/Xerox/xr_42152.ppd \
	ppd/Xerox/xr_42191.ppd \
	ppd/Xerox/xr_42192.ppd \
	ppd/Xerox/xr_42201.ppd \
	ppd/Xerox/xr_42202.ppd \
	ppd/Xerox/xr_42203.ppd \
	ppd/Xerox/xr_42204.ppd \
	ppd/Xerox/xr_42301.ppd \
	ppd/Xerox/xr_42302.ppd \
	ppd/Xerox/xr_45051.ppd \
	ppd/Xerox/xr_45171.ppd \
	ppd/Xerox/xr_45201.ppd \
	ppd/Xerox/xr_49001.ppd \
	ppd/Xerox/xr_49151.ppd \
	ppd/Xerox/xr_49201.ppd \
	ppd/Xerox/xr_88081.ppd \
	ppd/Xerox/xr_88121.ppd \
	ppd/Xerox/xrc55dp0.ppd \
	ppd/Xerox/xrd61351.ppd \
	ppd/Xerox/xrd61354.ppd \
	ppd/Xerox/xrd61801.ppd \
	ppd/Xerox/xrdt0904.ppd \
	ppd/Xerox/xrdt0905.ppd \
	ppd/Xerox/xrdt1354.ppd \
	ppd/Xerox/xrdt1355.ppd \
	ppd/Xerox/xrdtnsp1.ppd \
	ppd/Xerox/xrn17000.ppd \
	ppd/Xerox/xrn24320.ppd \
	ppd/Xerox/xrx_sw25.ppd \
#

MY_INSTALL_DIR := $(INSTALL_ETC)

include $(SDIR_TEMPLATES)/ZipTemplate.mk

SOURCE := Header
SRC_DIR := src/addons/print_server/postscript
TARGET := $(INSTALL_ADD_ONS)/Print/$(SOURCE)
PERMISSIONS := $(PERMS_SYSTEM_DATA)
include $(SDIR_TEMPLATES)/DataFileTemplate.mk

