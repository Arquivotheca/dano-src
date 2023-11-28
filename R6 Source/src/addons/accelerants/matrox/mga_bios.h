//////////////////////////////////////////////////////////////////////////////
// BIOS Structures
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Typedefs //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// "MGABiosInfo", used for the original Millennium.

typedef struct
{
  // Length of this structure in bytes
  ushort StructLen;

  // Unique number identifying the product type
  // 0 : MGA-S1P20 (2MB base with 175MHz Ramdac)
  // 1 : MGA-S1P21 (2MB base with 220MHz Ramdac)
  // 2 : Reserved
  // 3 : Reserved
  // 4 : MGA-S1P40 (4MB base with 175MHz Ramdac)
  // 5 : MGA-S1P41 (4MB base with 220MHz Ramdac)
  ushort ProductID;

  uchar SerNo[10];    // Serial number of the board

  // Manufacturing date of the board (at product test)
  // Format: yyyy yyym mmmd dddd
  ushort ManufDate;

  ushort ManufId;     // Identification of manufacturing site

  // Number and revision level of the PCB
  // Format: nnnn nnnn nnnr rrrr
  //         n = PCB number ex:576 (from 0->2047)
  //         r = PCB revision      (from 0->31)
  ushort PCBInfo;

  ushort PMBInfo;     // Identification of any PMBs

  // Bit  0- 7 : Ramdac speed (0=175MHz, 1=220MHz)
  // Bit  8-15 : Ramdac type  (0=TVP3026, 1=TVP3027)
  ushort RamdacType;

  ushort PclkMax;     // Maximum PCLK of the ramdac
  ushort LclkMax;     // Maximum LDCLK supported by the WRAM memory
  ushort ClkBase;     // Maximum MCLK of base board
  ushort Clk4MB;      // Maximum MCLK of 4Mb board
  ushort Clk8MB;      // Maximum MCLK of 8Mb board
  ushort ClkMod;      // Maximum MCLK of board with multimedia module
  ushort TestClk;     // Diagnostic test pass frequency
  ushort VGAFreq1;    // Default VGA mode1 pixel frequency
  ushort VGAFreq2;    // Default VGA mode2 pixel frequency
  ushort ProgramDate; // Date of last BIOS programming/update
  ushort ProgramCnt;  // Number of times BIOS has been programmed
  ulong Options;      // Support for up to 32 hardware/software options
  ulong FeatFlag;     // Support for up to 32 hardware/software features
  ushort VGAClk;      // Definition of VGA mode MCLK
  ushort StructRev;   // Indicate the revision level of this header struct

  ushort Reserved[3];
} BIOS_2064;


//////////////////////////////////////////////////////////////////////////////
//   This structure taken directly from Matrox PInS documentation, with
// the datatypes changed to match BeOS conventions

typedef struct
{
  uint16 PinID;         /*  Product Information Structure ID (412Eh) */
  /*  */
  /*  Note: This field MUST be programmed with the  */
  /*        16-bit value 412Eh. It represents a 5-bit */
  /*        ASCII encoding of the string "PIN". */
        
  uint8 StructLen;      /*  Product Information Structure length (in bytes) */
  /*  */
  /*  Note: As a simple means of verification, */
  /*        software should verify this length */
  /*        against the revision level given */
  /*        in the field StructRev. If the two do */
  /*        not match (go together), an error */
  /*        message should be displayed. */
  /*  */
  /*  Programmed to the length of this structure  */
  /*  (in bytes): 40h */
        
  uint8 Rsvd1;          /*  Reserved  */
  /*  */
  /*  This field is currently not used and must be */
  /*  programmed to FFh. */
        
  uint16 StructRev;     /*  Indicate the revision level of this */
  /*  information structure: */
  /*  */
  /*  Format: */
  /*          MMMMMMMM mmmmmmmm */
  /*  */
  /*  Where: */
  /*          MM = Major number (binary) */
  /*          mm = minor number (binary) */
  /*  */
  /*  e.g. */
  /*  */
  /*  For this version (2.0) of the structure: */
  /*  */
  /*          00000010 00000000 */
  /*  */
  /*             02       00 */
  /*  */
  /*                 0200h */
        
  uint16 ProgramDate;   /*  Date on which BIOS was last programmed/updated */
  /*  */
  /*  Format: */
  /*  */
  /*  yyyy yyym mmmd dddd */
  /*  */
  /*  y = Year  (7 bits - valid values: 0 -> 99) */
  /*  m = Month (4 bits - valid values: 1 -> 12) */
  /*  d = Date  (5 bits - valid values: 1 -> 31) */
  /*  */
  /*  Note: If not used, set this field to FFFFh */
        
  uint16 ProgramCnt;    /*  Number of times the BIOS has been programmed */
  /*  */
  /*  Note: This number should be set to 1 during the */
  /*        manufacturing test and then incremented */
  /*        EVERY TIME the BIOS is reprogrammed. */
  /*  */
  /*  Note: If not used, set this field to FFFFh */
        
  uint16 ProductID;     /*  Unique ID which identifies the product */
  /*  */
  /*  CCCCrrrr VVVooooo */
  /*    |   |   |   |  */
  /*    |   |   |   +------ OEM_ID */
  /*    |   |   +---------- Variation */
  /*    |   +-------------- Region */
  /*    +------------------ Channel */
  /*   */
  /*  Channel (4 bits): Sales channel through  */
  /*                    which board is sold. */
  /*  */
  /*  Region (4 bits): Sales region for which */
  /*                   the board is destined. */
  /*  */
  /*  Variation (3 bits): "SubType" of board. Together */
  /*                      with Channel, allows software */
  /*                      to identify the board type. */
  /*  */
  /*  OEM_ID (5 bits): This field is used mainly by OEMs  */
  /*                   not wishing or not needing to use  */
  /*                   the SubsytemVendorID and Sub- */
  /*                   SystemID fields. If OEM_ID is  */
  /*                   unused, must be programmed to 0h. */
  /*  */
  /*  Channel */
  /*  ------- */
  /*  */
  /*     0  - Retail */
  /*     1  - Bulk */
  /*     2  - OEM */
  /*     3  - VAR */
  /*     4  - Government */
  /*     5  - Corporate */
  /*     6  - MES (Video)  */
  /*     7  - MES (Imaging) */
  /*   8-15 - Reserved */
  /*   */
  /*   */
  /*  Region */
  /*  ------ */
  /*  */
  /*     0  - Americas (includes US and South America) */
  /*     1  - Europe */
  /*     2  - Far East */
  /*     3  - Canada */
  /*   4-15h - Reserved */
  /*  */
  /*  */
  /*  Variation   Retail  MES(Video)  MES(Imaging)   */
  /*  -------------------------------------------- */
  /*       */
  /*     0       Regular   Digimix      Pulsar */
  /*     1         DIP   DigiDisplay    Corona    */
  /*     2          -         -            -     */
  /*     3          -         -            -     */
  /*     4          -         -            -     */
  /*     5          -         -            -     */
  /*     6          -         -            -     */
  /*     7          -         -            -     */
  /*    */
  /*  Note: fields marked with '-' are reserved and should */
  /*        not be used. */
  /*  */
  /*  */
  /*  OEM_ID      OEM selection */
  /*  ------------------------- */
  /*     0        Reserved  */
  /*     :                : */
  /*     8        Reserved */
  /*     9        Undefined */
  /*     :           : */
  /*    31       Undefined */

  uint8 SerNo[16];      /*  PCB Serial Number */
  /*   */
  /*  Format: ASCIIZ (null-terminated) string */
  /*  */
  /*  Serial numbers less than 16 characters must be */
  /*  left-justified within the field and terminated */
  /*  by a NULL character (that is, any unused bytes */
  /*  should be at the *end* of the string)  */
  /*   */
  /*  Serial numbers exactly 16 characters in length */
  /*  do not require the terminating NULL character. */
  /*  */
  /*  Serial numbers greater than 16 bytes must be  */
  /*  truncated to 16 bytes and stored here *without* */
  /*  a terminating NULL character. */
  /*  */
  /*  Default: Each byte in this field should be */
  /*           programmed to FFh. */
        
  uint8 PLInfo[6];      /*  Part List Identification */
  /*   */
  /*  Format: ASCIIZ (null-terminated) string */
  /*  */
  /*  Permits the identification of the exact version of */
  /*  the parts list used to fabricate the board. The  */
  /*  parts list is ASCII-coded and placed in this field */
  /*  with a terminating NULL character. */
  /*  */
  /*  Note: If not used, set this field to FFh */
        
  uint16 PCBInfo;               /*  PCB Number and Revision level */
  /*  */
  /*  Format: */
  /*  */
  /*  nnnn nnnn nnnn rrrr */
  /*  */
  /*  n = PCB number e.g. 576 (from 0 -> 4095) */
  /*  r = PCB revision        (from 0 -> 15) */
  /*  */
  /*  Note: If not used, set this field to FFFFh */
        
  uint32 FeatFlag;      /*  32 feature bits which specify the value of certain */
  /*  options applicable to this PCB. */
  /*  */
  /*  Programmed to FFFFFFFFh by default */
  /*  */
  /*  Definitions: */
  /*  */
  /*  None as yet. */
        
  uint8 RamdacType;     /*  Ramdac Type */
  /*  */
  /*  Used to indicate to software the type of RAMDAC. */
  /*  */
  /*  Values: */
  /*   */
  /*      0h : Internal */
  /*     26h : TVP3026 */
  /*     27h : TVP3027 */
  /*     30h : TVP3030 */
  /*     33h : TVP3033 */
  /*     FFh : Unknown/Unprogrammed */
  /*   */
  /*  Note: The default for this field is FFh */
        
  uint8 RamdacSpeed;    /*  Ramdac Speed */
  /*  */
  /*  Used to indicate to software the rated speed */
  /*  of the RAMDAC: */
  /*  */
  /*  Values: */
  /*   */
  /*    RatedSpeed - 100d */
  /*       e.g. For a -175 DAC, program this field to */
  /*            75d (4Bh) */
  /*  */
  /*  Note: If not used, set this field to FFh */
        
  uint8 PclkMax;                /*  Maximum RAMDAC Speed */
  /*  */
  /*  A non-FFh value in this field indicates the  */
  /*  maximum pixel clock supported by the RAMDAC (This */
  /*  field overrides the information given in */
  /*  RamdacType->Speed). */
  /*  */
  /*  Format: */
  /*   */
  /*    DACSpeed - 100d */
  /*       e.g. To run a -175 DAC up to 185 MHz,  */
  /*            program this field to 85d (55h) */
  /*  */
  /*  Note: If not used, set this field to FFh */
        
  uint8 ClkGE;          /*  Maximum Graphics Engine clock for base board */
  /*  */
  /*  This field provides the clock to be used as the */
  /*  GRAPHICS clock for the on-board controller. */
  /*  */
  /*  Notes:  */
  /*       - It should be programmed based on board */
  /*         characteristics (memory type/speed) and */
  /*         should be used both in Hires mode as well */
  /*         as VGA mode (see VGAClk). */
  /*  */
  /*       - For MGA-1064SG-based boards (e.g. Mystique), */
  /*         the Graphics Engine clock (ClkGE) is */
  /*         derived from the System PLL inside the ASIC */
  /*         (Generally, GCLK is equal to SYSPLL / 3). */
  /*         For MGA-2164W-based boards (e.g. Mistral), */
  /*         the Graphics Engine clock is provided by */
  /*         the external RAMDAC (e.g. TVP3026B).    */
  /*          */
  /*  Format:  */
  /*  */
  /*   ClkGE */
  /*  */
  /*  e.g. */
  /*  0 -> 255  ==> 0 MHz -> 255 MHz */
  /*  */
  /*  Usage of ClkGE and ClkMem (MGA-1064SG): */
  /*  -------------------------------------- */
  /*  To simplify the interpretation of these two  */
  /*  fields, the following algorithm is proposed. */
  /*  It gives us better granularity in selecting */
  /*  the graphics and memory clock frequencies as */
  /*  well as reduces the amount of data needing to */
  /*  be programmed. */
  /*  */
  /*  If (ClkGE != FF) and (ClkMem == FF) */
  /*      Program System PLL = 3 * ClkGE; */
  /*      Graphics clock divider = 3; */
  /*      Memory clock divider = 2; */
  /*  ElseIf (ClkGE == FF) and (ClkMem != FF) */
  /*      Program System PLL = 2 * ClkMem; */
  /*      Graphics clock divider = 3; */
  /*      Memory clock divider = 2; */
  /*  ElseIf (ClkGE == ClkMem) and (ClkGE != FF) */
  /*      Program System PLL = ClkMem; */
  /*      Graphics clock divider = 1; */
  /*      Memory clock divider = 1; */
  /*  Else */
  /*      Program System PLL = 132; */
  /*      Graphics clock divider = 3; */
  /*      Memory clock divider = 2; */
  /*  */
  /*  Usage of ClkGE and ClkMem (MGA-2164W): */
  /*  ---------------------------------------- */
  /*  The following algorithm is proposed for the */
  /*  programming of the Graphic engine clock on  */
  /*  MGA-2164W-based products (e.g. Millennium-Pro). */
  /*  Note that on these products there is a single */
  /*  clock for the controller on which the memory  */
  /*  timings are based and the clock generation  */
  /*  circuitry is located in the RAMDAC. This */
  /*  significantly simplifies the clock programming */
  /*  algorithm. */
  /*  */
  /*  If (ClkGE != FF) and (ClkMem == FF) */
  /*      Program MCLK = ClkGE; */
  /*  ElseIf (ClkGE == FF) and (ClkMem != FF) */
  /*      Program MCLK = ClkMem; */
  /*  ElseIf (ClkGE == ClkMem) and (ClkGE != FF) */
  /*      Program MCLK = ClkGE; */
  /*  Else */
  /*      Program MCLK = 60; */
  /*  */
  /*  Note: This field must be programmed in accordance */
  /*        with these guidelines. */
        
  uint8 ClkMem;         /*  Maximum Memory clock frequency for base board */
  /*  */
  /*  This field provides the clock to be used as the */
  /*  Memory clock for the on-board memory. See the  */
  /*  ClkGE field for programming instructions... */
  /*  */
  /*  Notes:  */
  /*      - This value should be programmed based on */
  /*        board characteristics (memory type/speed) */
  /*        and should be used both in Hires mode as */
  /*        well as VGA mode (see VGAClk) */
  /*  */
  /*      - For MGA-1064SG-based designs (e.g. Mystique), */
  /*        the memory clock is derived from the System  */
  /*        PLL inside the ASIC (generally, MCLK is equal */
  /*        to SYSPLL / 2). */
  /*   */
  /*  Format:  */
  /*  */
  /*   ClkMem */
  /*  */
  /*  e.g. */
  /*  0 -> 255  ==> 0 MHz -> 255 MHz */
  /*  */
  /*  Note(s):  */
  /*  - This field must be programmed in accordance */
  /*    with these guidelines. */
  /*  - In the case of independent memory and system */
  /*    clocks (i.e. Mystique) care must be taken in */
  /*    selecting the two clocks to be used.  */
        
  uint8 Clk4MB;         /*  Maximum Graphic Engine (GCLK) clock for a board */
  /*  configuration where the total memory is 4 MB. */
  /*  */
  /*  Format:  */
  /*  */
  /*   Clk4MB */
  /*  */
  /*  e.g. */
  /*  0 -> 255  ==> 0 MHz -> 255 MHz */
  /*  */
  /*  Notes:  */
  /*      - If not used, set this field to FFh */
  /*      - If this field is programmed to FFh, */
  /*        software will use the value of ClkGE */
        
  uint8 Clk8MB;         /*  Maximum Graphic Engine (GCLK) clock for a board  */
  /*  configuration where the total memory is 8 MB.  */
  /*  */
  /*  i.e. */
  /*  - 2 MB base PCB with a 6MB module, or */
  /*  - 4 MB base PCB with a 4MB module, or */
  /*  - 8 MB base PCB (no module) (*) */
  /*  */
  /*  Format:  */
  /*  */
  /*   Clk8MB */
  /*  */
  /*  e.g. */
  /*  0 -> 255  ==> 0 MHz -> 255 MHz */
  /*  */
  /*  Notes:  */
  /*      - If not used, set this field to FFh */
  /*      - If this field is programmed to FFh, */
  /*        software will use the value of ClkGE */
  /*  (*) - For an 8 MB base configuration, this */
  /*        field need not be used */
        
  uint8 ClkMod;         /*  Maximum Graphic Engine (GCLK) clock for a base */
  /*  PCB with a module other than a memory upgrade  */
  /*  (e.g. Multimedia) */
  /*  */
  /*  Format:  */
  /*  */
  /*   ClkMod */
  /*  */
  /*  e.g. */
  /*  0 -> 255  ==> 0 MHz -> 255 MHz */
  /*  */
  /*  Notes:  */
  /*      - If not used, set this field to FFh */
  /*      - If this field is programmed to FFh, */
  /*        software will use the value of ClkGE */
        
  uint8 TestClk;                /*  Frequency at which test diagnostic was run */
  /*  (and passed) */
  /*  */
  /*  Format:  */
  /*  */
  /*   TestClk */
  /*  */
  /*  e.g. */
  /*  0 -> 255  ==> 0 MHz -> 255 MHz */
  /*  */
  /*  Note: If not used, set this field to FFh */
        
  uint8 VGAFreq1;               /*  Default Frequency for mode VGA1 */
  /*  */
  /*  Format:  */
  /*  */
  /*   VGAFreq1 * 4 */
  /*  */
  /*  e.g. */
  /*  0 -> 255  ==> 0 MHz -> 63.75 MHz */
  /*  */
  /*  Note: If not used, set this field to FFh */
        
  uint8 VGAFreq2;               /*  Default Frequency for mode VGA2 */
  /*  */
  /*  Format:  */
  /*  */
  /*   VGAFreq2 * 4 */
  /*  */
  /*  e.g. */
  /*  0 -> 255  ==> 0 MHz -> 63.75 MHz */
  /*  */
  /*  Note: If not used, set this field to FFh */
        
  uint8 MCTLWTST;               /*  Memory Control Wait State */
  /*  */
  /*  This field specifies the parameters to program */
  /*  into the MCTLWTST register of Mystique. Note */
  /*  that on Mistral-based boards, these values are  */
  /*  not used and should be programmed to '1's (e.g. 0xFF) */
  /*   */
  /*  Format: */
  /*  */
  /*  xxxx mmdc */
  /*    |  ||||  */
  /*    |  |||+--- casltncy */
  /*    |  ||+---- rcdelay */
  /*    |  ++----- rasmin */
  /*    | */
  /*    +--------- Reserved */
  /*   */
  /*  casltncy: CAS/ Latency */
  /*   */
  /*     0: 2 clocks */
  /*     1: 3 clocks (default) */
  /*   */
  /*  rcdelay: RAS/-to-CAS/ Delay */
  /*   */
  /*     0: 2 clocks */
  /*     1: 3 clocks (default) */
  /*   */
  /*  rasmin: Minimum RAS/ active time  */
  /*   */
  /*     0: 4 clocks */
  /*     1: 5 clocks */
  /*     2: 6 clocks */
  /*     3: 7 clocks (default) */
  /*   */
  /*  Reserved: Unused bits (program to '1') */
  /*   */
  /*  Note: All unused bits must be programmed to '1' */
        
  uint8 VidCtrl;                /*  Analog Video Setup Information */
  /*  */
  /*  This field specifies video-related parameters */
  /*  that are used by the XGENCTRL and CRTEXT3  */
  /*  registers of Mystique. */
  /*   */
  /*  Format: */
  /*  */
  /*  xxsc xxxp */
  /*   |||  | |  */
  /*   |||  | +--- Pedestal */
  /*   |||  +----- Reserved */
  /*   ||+-------- CompSyncEn */
  /*   |+--------- SyncOnGreenEn */
  /*   +---------- Reserved */
  /*   */
  /*  SyncOnGreenEn: Enable for Sync on Green */
  /*   */
  /*     0: Enabled */
  /*     1: Disabled (default) */
  /*   */
  /*     Note: This bit can be directly programmed */
  /*           into the XGENCTL[iogsyncgen] field. */
  /*   */
  /*  CompSyncEn: Composite Sync Enable */
  /*   */
  /*     0: Separate Syncs (default) */
  /*     1: Composite Syncs */
  /*   */
  /*     Note: This bit can be found the Mystique */
  /*           register CRTCEXT3[csyncen]. It is  */
  /*           defined identically. */
  /*   */
  /*  Pedestal: Video Pedestal  */
  /*   */
  /*     0: 0 IRE (default) */
  /*     1: 7.5 IRE */
  /*   */
  /*     Note: This bit can be found the Mystique */
  /*           register XGENCTRL and is defined  */
  /*           identically (Mystique). */
  /*   */
  /*  Reserved: Unused bits (program to '1') */
  /*   */
  /*  Notes:  */
  /*   - All unused bits must be programmed to '1' */
  /*   - For Mistral-based boards, the register locations */
  /*     of these bits will be different */
        
  uint8 Clk12MB;                /*  Maximum Graphic Engine (GCLK) clock for a board  */
  /*  configuration where the total memory is 12 MB.  */
  /*  */
  /*  i.e. */
  /*  - 4 MB base PCB with a 8MB module, or */
  /*  - 8 MB base PCB with a 4MB module */
  /*  */
  /*  Format:  */
  /*  */
  /*   Clk12MB */
  /*  */
  /*  e.g. */
  /*  0 -> 255  ==> 0 MHz -> 255 MHz */
  /*  */
  /*  Notes:  */
  /*      - If not used, set this field to FFh */
  /*      - If this field is programmed to FFh, */
  /*        software will use the value of ClkGE */
        
  uint8 Clk16MB;                /*  Maximum Graphic Engine (GCLK) clock for a board  */
  /*  configuration where the total memory is 16 MB.  */
  /*  */
  /*  i.e. */
  /*  - 8 MB base PCB with an 8MB module */
  /*  */
  /*  Format:  */
  /*  */
  /*   Clk16MB */
  /*  */
  /*  e.g. */
  /*  0 -> 255  ==> 0 MHz -> 255 MHz */
  /*  */
  /*  Notes:  */
  /*      - If not used, set this field to FFh */
  /*      - If this field is programmed to FFh, */
  /*        software will use the value of ClkGE */
        
  uint8 Reserved[8];    /*  These are reserved bytes which are currently */
  /*  undefined (reserved) and must all be programmed */
  /*  to FFh. */
        
  uint8 PinCheck;               /*  PInS Checksum */
  /*   */
  /*  This field *must* be programmed with a value such */
  /*  that when added to the 8-bit sum of the preceding */
  /*  63 bytes, the value is 0 (This is a checksum for */
  /*  the PInS structure). */
        
} BIOS_1064; // "ParamMGA", used for the Mystique and the Millennium II.


//////////////////////////////////////////////////////////////////////////////
// G100/G200 BIOS Structure

typedef struct
{
  uint16 PinID;       // LE constant 0x412E
  uint8  StructLen;   // length of structure.  Should be 0x40 (64 bytes)
  uint8  Reserved1;
  uint16 StructRev;   // LE constant 0x0300 or bigger
  uint16 ProgramDate; // Date BIOS programmed
  uint16 ProgramCnt;  // number of times BIOS programmed
  uint16 ProductID;   // info about channel, region, and type
  uint8  SerNo[16];   // serial number of card
  uint8  PLInfo[6];   // parts list info
  uint16 PCBInfo;     // PCB number and rev

  // Maximum VCO Frequncy.  A non-FFh value in this field indicates the
  // maximum VCO frequency supported by the RAMDAC.  Format is
  // MaxVCOSpeed - 100d.  Eg: for 175MHz write 75d (4Bh)
  uint8  VCOMax;

  // max pixel clock in 8bpp mode.  Format is DACSpeed - 100d
  uint8  PclkMax8;

  uint8  PclkMax16;      // for 16bpp.  See PclkMax8
  uint8  PclkMax24;      // for 24bpp.  See PclkMax8
  uint8  PclkMax32;      // for 32bpp.  See PclkMax8
  uint8  PclkVGA1;       // frequency for std VGA modes
  uint8  PclkVGA2;       // frequency for std VGA modes
  uint8  VGAClk;         // gclk for std VGA modes

  // value to be programmed for gclk in HiRes mode when there is no
  // memory module.  0 -> 255MHz. Default 63MHz.
  uint8  HiResGClk;

  // gclk value when 2MB module present.  Default 0xff means use HiResGClk
  uint8  HiResGClk2;

  // gclk value when 4MB module present.  Default 0xff means use HiResGClk
  uint8  HiResGClk4;

  // gclk value when 8MB module present.  Default 0xff means use HiResGClk
  uint8  HiResGClk8;

  // value to be loaded directly into the MCTLWTST register.
  // Default 0x02032521 for G100, 0x00244CA1 for G200
  uint32 MCTLWTST;

  // Clock Dividers for internal clock generation.  The values here are
  // loaded into the corresponding bit field of the OPTION register.  The
  // definitions of fmclkdiv, mclkdiv, and gclkdiv bits can be found in the
  // appropriate ASIC specification; the definition of synmtype is given
  // below.  Note: the fmclkdiv field applies only to G100.  Format:
  //
  // Bit 7 - 0
  // Bit 6 - 0
  // Bit 5 - fref  frequency ref.  0 == 27.00, 1 == 14.318
  // Bit 4 - syncmtype  Synchronous Memory Type.  0 == SGRAM, 1 == SDRAM
  // Bit 3 - pllswap  Internal PLL swap.  0 == Do NOT swap.  1 == Swap.
  // Bit 2 - fmclkdiv  See G100 docs.
  // Bit 1 - mclkdiv   See Gx00 docs.
  // Bit 0 - gclkdiv   See Gx00 docs.
  uint8  ClkDiv;

  // Analog video setup info.  This field specifies video-related parameters
  // that are used by the XGENCTRL and CRTEXT3 registers.  Format:
  //
  // Bit 7,6 - reserved
  // Bit 5 - SyncOnGreen.  0 == Enabled, 1 == Disabled
  // Bit 4 - CompSyncEn  Composite Sync Enable.
  //   0 == Separate syncs (default)
  //   1 == Composite syncs
  // Bit 3,2 - PLinkClk
  //   0,1,2 == Reserved
  //   3 == 112MHz
  // Bit 1 - reserved
  // Bit 0 - Pedestal  Video Pedestal  0 == 0 IRE, 1 == 7.5 IRE
  //
  // Default value: 0xEE
  uint8  VidCtrl;

  // Memory Type Definition.  This field defines the type and size of memory
  // that is installed on the base board, as well as parameter information.
  // For details refer to the Gx00 docs.
  //
  // G100
  //  Bits 7,4 - Memrclkd : Memory Read Clock Delay
  //  Bits 3,2 - Msize : Memory Device Size
  //  Bits 1,0 - Mtype : Memory Device Type
  //
  // G200
  //  Bits 7,3 - zero filled
  //  Bits 2,0 - memconfig : OPTION(memconfig)
  uint8  MemType;

  // This field defines the contents of the CRTCEXT6 register in both VGA and
  // HiRes modes, as well as the quantity of memory installed on the base
  // board (granularity of 2MB).  Note: The fields Hpri_hres and Hpri_vga
  // apply only to G100 - the field Memqty applies to both G100 and G200
  // devices.  Format:
  //
  //  Bits 7,6 - MemQty: 0 == 2MB, 1 == 4MB, 2 == 8MB, 3 == reserved.
  //  Bits 5,3 - Hpri_hres value to be programmed.
  //  Bits 2,0 - Hpri_vga value to be programmed.
  uint8  MemParam;

  // G200 only.  These fields are programmed directly into the corresponding
  // fields in the G200 MEMRDBK register.
  //
  //  Bits 7,4 - mclkbrd1
  //  Bits 3,0 - mclkbrd0
  //
  //  Default 0x88
  uint8  Memrdbk1;

  // G200 only.  These fields are programmed directly into the coresponding
  // fields in the G200 MEMRDBK register.  See the G200 docs for details.
  //
  //  Bits 7,4 - mrsopcod
  //  Bits 3,2 - zero filled
  //  Bits 1,0 - strmfctl
  //
  //  Default 0x00
  uint8  Memrdbk2;

  // G200 only.  Programmed directly into the OPTION2 register.  Format:
  //
  //  Bits 7,6 - zero filled
  //  Bit  5   - wclkdiv
  //  Bit  4   - nowclkdiv
  //  Bit  3   - nomclkdiv
  //  Bit  2   - nogclkdiv
  //  Bits 1,0 - mbuftype
  //
  //  Default 0x00
  uint8  Option2;

  // This byte allows software to identify options which have been assembled
  // onto the based graphics board (eg: MAVEN).  For each option, a '1' bit
  // signifies that the option is *NOT* installed.  Format:
  //
  //  Bit 7 - 1 (no option)
  //  Bit 6 - Panel Link
  //  Bit 5 - Audio Support
  //  Bit 4 - TV-Tuner
  //  Bit 3 - VIN (Decoder)
  //  Bit 2 - MJPEG
  //  Bit 1 - DVD
  //  Bit 0 - TV Out (MAVEN)
  //
  //  Default 0xff (nothing installed)
  uint8  FactoryOpt;

  // defines gclk speed used in 3D modes. 0 -> 254MHz.  Default
  // value of 0xff means use 2D glck instead
  uint8  Clk3D;

  // G200 only.  Clock dividers for internal clock generation when in 3D
  // mode.  The values here are loaded into the corresponding bit field of
  // the OPTION and OPTION2 registers.  Format:
  //
  // Bit 7 - gclkdiv
  // Bit 6 - mclkdiv
  // Bit 5 - wclkdiv
  // Bit 4 - nowclkdiv
  // Bit 3 - nomclkdiv
  // Bit 2 - nogclkdiv
  // Bits 1,0 - set to '11'
  //
  // Default 0xff
  uint8  ClkDiv3D;

  // This byte is used to define the derating value used when add-on memory
  // modules are present.  The derating is done in steps of 2MHz according to
  // the following formula:
  //
  // 4MBFactor: Derating used when a 4MB upgrade is present.
  // 8MBFactor: Derating used when an 8MB upgrade is present.
  //
  // GCLK = Clk3D - 2 * (15 - xMBFactor)
  //
  // Format:
  //
  // Bits 7,4 - 8MBFactor
  // Bits 3,0 - 4MBFactor
  //
  // Default 0xff (leave factor unchanged)
  uint8  Derate3D;

  uint8  PinCheck;   // checksum for PInS data
} BIOS_G100;


//////////////////////////////////////////////////////////////////////////////
// Card Static Data

typedef struct
{
  uint16 DeviceID;    // The PCI Device ID.
  uint16 Revision;    // The card rev. number.
  char   Name[32];    // A sugary string for the user.
  char   Chipset[32]; // A chipset name.
} CARD_STATIC_DATA;


//////////////////////////////////////////////////////////////////////////////
// Externs ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Card Static Data
//    This structure contains some static data for cards; the rev. number,
// a couple of text strings, that kind of thing.  It's cosmetic for the most
// part.  It may get compiled in later.

//extern CARD_STATIC_DATA CardStaticData;


//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
