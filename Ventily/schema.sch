<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE eagle SYSTEM "eagle.dtd">
<eagle version="9.6.0">
<drawing>
<settings>
<setting alwaysvectorfont="no"/>
<setting verticaltext="up"/>
</settings>
<grid distance="0.1" unitdist="inch" unit="inch" style="lines" multiple="1" display="no" altdistance="0.01" altunitdist="inch" altunit="inch"/>
<layers>
<layer number="1" name="Top" color="4" fill="1" visible="no" active="no"/>
<layer number="16" name="Bottom" color="1" fill="1" visible="no" active="no"/>
<layer number="17" name="Pads" color="2" fill="1" visible="no" active="no"/>
<layer number="18" name="Vias" color="2" fill="1" visible="no" active="no"/>
<layer number="19" name="Unrouted" color="6" fill="1" visible="no" active="no"/>
<layer number="20" name="Dimension" color="15" fill="1" visible="no" active="no"/>
<layer number="21" name="tPlace" color="7" fill="1" visible="no" active="no"/>
<layer number="22" name="bPlace" color="7" fill="1" visible="no" active="no"/>
<layer number="23" name="tOrigins" color="15" fill="1" visible="no" active="no"/>
<layer number="24" name="bOrigins" color="15" fill="1" visible="no" active="no"/>
<layer number="25" name="tNames" color="7" fill="1" visible="no" active="no"/>
<layer number="26" name="bNames" color="7" fill="1" visible="no" active="no"/>
<layer number="27" name="tValues" color="7" fill="1" visible="no" active="no"/>
<layer number="28" name="bValues" color="7" fill="1" visible="no" active="no"/>
<layer number="29" name="tStop" color="7" fill="3" visible="no" active="no"/>
<layer number="30" name="bStop" color="7" fill="6" visible="no" active="no"/>
<layer number="31" name="tCream" color="7" fill="4" visible="no" active="no"/>
<layer number="32" name="bCream" color="7" fill="5" visible="no" active="no"/>
<layer number="33" name="tFinish" color="6" fill="3" visible="no" active="no"/>
<layer number="34" name="bFinish" color="6" fill="6" visible="no" active="no"/>
<layer number="35" name="tGlue" color="7" fill="4" visible="no" active="no"/>
<layer number="36" name="bGlue" color="7" fill="5" visible="no" active="no"/>
<layer number="37" name="tTest" color="7" fill="1" visible="no" active="no"/>
<layer number="38" name="bTest" color="7" fill="1" visible="no" active="no"/>
<layer number="39" name="tKeepout" color="4" fill="11" visible="no" active="no"/>
<layer number="40" name="bKeepout" color="1" fill="11" visible="no" active="no"/>
<layer number="41" name="tRestrict" color="4" fill="10" visible="no" active="no"/>
<layer number="42" name="bRestrict" color="1" fill="10" visible="no" active="no"/>
<layer number="43" name="vRestrict" color="2" fill="10" visible="no" active="no"/>
<layer number="44" name="Drills" color="7" fill="1" visible="no" active="no"/>
<layer number="45" name="Holes" color="7" fill="1" visible="no" active="no"/>
<layer number="46" name="Milling" color="3" fill="1" visible="no" active="no"/>
<layer number="47" name="Measures" color="7" fill="1" visible="no" active="no"/>
<layer number="48" name="Document" color="7" fill="1" visible="no" active="no"/>
<layer number="49" name="Reference" color="7" fill="1" visible="no" active="no"/>
<layer number="51" name="tDocu" color="6" fill="1" visible="no" active="no"/>
<layer number="52" name="bDocu" color="7" fill="1" visible="no" active="no"/>
<layer number="88" name="SimResults" color="9" fill="1" visible="yes" active="yes"/>
<layer number="89" name="SimProbes" color="9" fill="1" visible="yes" active="yes"/>
<layer number="90" name="Modules" color="5" fill="1" visible="yes" active="yes"/>
<layer number="91" name="Nets" color="2" fill="1" visible="yes" active="yes"/>
<layer number="92" name="Busses" color="1" fill="1" visible="yes" active="yes"/>
<layer number="93" name="Pins" color="2" fill="1" visible="no" active="yes"/>
<layer number="94" name="Symbols" color="4" fill="1" visible="yes" active="yes"/>
<layer number="95" name="Names" color="7" fill="1" visible="yes" active="yes"/>
<layer number="96" name="Values" color="7" fill="1" visible="yes" active="yes"/>
<layer number="97" name="Info" color="7" fill="1" visible="yes" active="yes"/>
<layer number="98" name="Guide" color="6" fill="1" visible="yes" active="yes"/>
</layers>
<schematic xreflabel="%F%N/%S.%C%R" xrefpart="/%S.%C%R">
<libraries>
<library name="relay" urn="urn:adsk.eagle:library:339">
<description>&lt;b&gt;Relays&lt;/b&gt;&lt;p&gt;
&lt;ul&gt;
&lt;li&gt;Eichhoff
&lt;li&gt;Finder
&lt;li&gt;Fujitsu
&lt;li&gt;HAMLIN
&lt;li&gt;OMRON
&lt;li&gt;Matsushita
&lt;li&gt;NAiS
&lt;li&gt;Siemens
&lt;li&gt;Schrack
&lt;/ul&gt;
&lt;author&gt;Created by librarian@cadsoft.de&lt;/author&gt;</description>
<packages>
<package name="G5LE" urn="urn:adsk.eagle:footprint:23966/1" library_version="5">
<description>&lt;b&gt;RELAY&lt;/b&gt;&lt;p&gt;
1 x switch, 10 A/120 V AC, 8 A/30 V ADC, Omron</description>
<wire x1="-12.065" y1="8.255" x2="10.541" y2="8.255" width="0.1524" layer="21"/>
<wire x1="10.541" y1="-8.255" x2="10.541" y2="8.255" width="0.1524" layer="21"/>
<wire x1="10.541" y1="-8.255" x2="-12.065" y2="-8.255" width="0.1524" layer="21"/>
<wire x1="-12.065" y1="8.255" x2="-12.065" y2="-8.255" width="0.1524" layer="21"/>
<wire x1="-5.08" y1="5.969" x2="-4.445" y2="5.969" width="0.1524" layer="21"/>
<wire x1="-4.445" y1="5.969" x2="-4.445" y2="0.635" width="0.1524" layer="21"/>
<wire x1="-4.445" y1="0.635" x2="-5.715" y2="0.635" width="0.254" layer="21"/>
<wire x1="-5.715" y1="0.635" x2="-5.715" y2="-0.635" width="0.1524" layer="21"/>
<wire x1="-3.175" y1="-0.635" x2="-3.175" y2="0.635" width="0.254" layer="21"/>
<wire x1="-3.175" y1="0.635" x2="-4.445" y2="0.635" width="0.254" layer="21"/>
<wire x1="-5.715" y1="-0.635" x2="-4.445" y2="-0.635" width="0.254" layer="21"/>
<wire x1="-4.445" y1="-0.635" x2="-4.445" y2="-5.969" width="0.1524" layer="21"/>
<wire x1="-4.445" y1="-0.635" x2="-3.175" y2="-0.635" width="0.254" layer="21"/>
<wire x1="-4.445" y1="-5.969" x2="-5.08" y2="-5.969" width="0.1524" layer="21"/>
<wire x1="-5.715" y1="-0.635" x2="-3.175" y2="0.635" width="0.1524" layer="21"/>
<wire x1="-2.54" y1="0" x2="4.064" y2="0" width="0.1524" layer="21"/>
<wire x1="4.064" y1="0" x2="4.699" y2="0.635" width="0.254" layer="21"/>
<wire x1="4.699" y1="3.7338" x2="4.699" y2="0.635" width="0.1524" layer="21"/>
<wire x1="4.699" y1="-0.635" x2="4.699" y2="-3.7592" width="0.1524" layer="21"/>
<wire x1="4.699" y1="0.635" x2="5.0292" y2="0.9398" width="0.254" layer="21"/>
<wire x1="-7.366" y1="0" x2="-6.35" y2="0" width="0.1524" layer="21"/>
<pad name="P" x="-9.525" y="0" drill="1.3208" shape="long"/>
<pad name="1" x="-7.493" y="-5.969" drill="1.3208" shape="long"/>
<pad name="2" x="-7.493" y="5.969" drill="1.3208" shape="long"/>
<pad name="O" x="4.699" y="5.969" drill="1.3208" shape="long"/>
<pad name="S" x="4.699" y="-5.969" drill="1.3208" shape="long"/>
<text x="12.827" y="-8.255" size="1.778" layer="25" ratio="10" rot="R90">&gt;NAME</text>
<text x="9.525" y="-7.62" size="1.778" layer="27" ratio="10" rot="R90">&gt;VALUE</text>
</package>
</packages>
<packages3d>
<package3d name="G5LE" urn="urn:adsk.eagle:package:24308/1" type="box" library_version="5">
<description>RELAY
1 x switch, 10 A/120 V AC, 8 A/30 V ADC, Omron</description>
<packageinstances>
<packageinstance name="G5LE"/>
</packageinstances>
</package3d>
</packages3d>
<symbols>
<symbol name="K" urn="urn:adsk.eagle:symbol:23941/1" library_version="5">
<wire x1="-3.81" y1="-1.905" x2="-1.905" y2="-1.905" width="0.254" layer="94"/>
<wire x1="3.81" y1="-1.905" x2="3.81" y2="1.905" width="0.254" layer="94"/>
<wire x1="3.81" y1="1.905" x2="1.905" y2="1.905" width="0.254" layer="94"/>
<wire x1="-3.81" y1="1.905" x2="-3.81" y2="-1.905" width="0.254" layer="94"/>
<wire x1="0" y1="-2.54" x2="0" y2="-1.905" width="0.1524" layer="94"/>
<wire x1="0" y1="-1.905" x2="3.81" y2="-1.905" width="0.254" layer="94"/>
<wire x1="0" y1="2.54" x2="0" y2="1.905" width="0.1524" layer="94"/>
<wire x1="0" y1="1.905" x2="-3.81" y2="1.905" width="0.254" layer="94"/>
<wire x1="-1.905" y1="-1.905" x2="1.905" y2="1.905" width="0.1524" layer="94"/>
<wire x1="-1.905" y1="-1.905" x2="0" y2="-1.905" width="0.254" layer="94"/>
<wire x1="1.905" y1="1.905" x2="0" y2="1.905" width="0.254" layer="94"/>
<text x="1.27" y="2.921" size="1.778" layer="96">&gt;VALUE</text>
<text x="1.27" y="5.08" size="1.778" layer="95">&gt;PART</text>
<pin name="2" x="0" y="-5.08" visible="pad" length="short" direction="pas" rot="R90"/>
<pin name="1" x="0" y="5.08" visible="pad" length="short" direction="pas" rot="R270"/>
</symbol>
<symbol name="U" urn="urn:adsk.eagle:symbol:23944/1" library_version="5">
<wire x1="3.175" y1="5.08" x2="1.905" y2="5.08" width="0.254" layer="94"/>
<wire x1="-3.175" y1="5.08" x2="-1.905" y2="5.08" width="0.254" layer="94"/>
<wire x1="0" y1="1.27" x2="2.54" y2="5.715" width="0.254" layer="94"/>
<wire x1="0" y1="1.27" x2="0" y2="0" width="0.254" layer="94"/>
<circle x="0" y="1.27" radius="0.127" width="0.4064" layer="94"/>
<text x="2.54" y="0" size="1.778" layer="95">&gt;PART</text>
<pin name="O" x="5.08" y="5.08" visible="pad" length="short" direction="pas" rot="R180"/>
<pin name="S" x="-5.08" y="5.08" visible="pad" length="short" direction="pas"/>
<pin name="P" x="0" y="-2.54" visible="pad" length="short" direction="pas" rot="R90"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="G5L" urn="urn:adsk.eagle:component:24582/2" prefix="K" library_version="5">
<description>&lt;b&gt;RELAY&lt;/b&gt;&lt;p&gt;
1 x switch, 10 A/120 V AC, 8 A/30 V ADC, Omron</description>
<gates>
<gate name="1" symbol="K" x="0" y="0" addlevel="must"/>
<gate name="2" symbol="U" x="17.78" y="-2.54" addlevel="always"/>
</gates>
<devices>
<device name="" package="G5LE">
<connects>
<connect gate="1" pin="1" pad="1"/>
<connect gate="1" pin="2" pad="2"/>
<connect gate="2" pin="O" pad="O"/>
<connect gate="2" pin="P" pad="P"/>
<connect gate="2" pin="S" pad="S"/>
</connects>
<package3dinstances>
<package3dinstance package3d_urn="urn:adsk.eagle:package:24308/1"/>
</package3dinstances>
<technologies>
<technology name="">
<attribute name="MF" value="" constant="no"/>
<attribute name="MPN" value="" constant="no"/>
<attribute name="OC_FARNELL" value="unknown" constant="no"/>
<attribute name="OC_NEWARK" value="unknown" constant="no"/>
<attribute name="POPULARITY" value="10" constant="no"/>
</technology>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
<library name="supply1" urn="urn:adsk.eagle:library:371">
<description>&lt;b&gt;Supply Symbols&lt;/b&gt;&lt;p&gt;
 GND, VCC, 0V, +5V, -5V, etc.&lt;p&gt;
 Please keep in mind, that these devices are necessary for the
 automatic wiring of the supply signals.&lt;p&gt;
 The pin name defined in the symbol is identical to the net which is to be wired automatically.&lt;p&gt;
 In this library the device names are the same as the pin names of the symbols, therefore the correct signal names appear next to the supply symbols in the schematic.&lt;p&gt;
 &lt;author&gt;Created by librarian@cadsoft.de&lt;/author&gt;</description>
<packages>
</packages>
<symbols>
<symbol name="+5V" urn="urn:adsk.eagle:symbol:26929/1" library_version="1">
<wire x1="1.27" y1="-1.905" x2="0" y2="0" width="0.254" layer="94"/>
<wire x1="0" y1="0" x2="-1.27" y2="-1.905" width="0.254" layer="94"/>
<text x="-2.54" y="-5.08" size="1.778" layer="96" rot="R90">&gt;VALUE</text>
<pin name="+5V" x="0" y="-2.54" visible="off" length="short" direction="sup" rot="R90"/>
</symbol>
<symbol name="GND" urn="urn:adsk.eagle:symbol:26925/1" library_version="1">
<wire x1="-1.905" y1="0" x2="1.905" y2="0" width="0.254" layer="94"/>
<text x="-2.54" y="-2.54" size="1.778" layer="96">&gt;VALUE</text>
<pin name="GND" x="0" y="2.54" visible="off" length="short" direction="sup" rot="R270"/>
</symbol>
<symbol name="+12V" urn="urn:adsk.eagle:symbol:26931/1" library_version="1">
<wire x1="1.27" y1="-1.905" x2="0" y2="0" width="0.254" layer="94"/>
<wire x1="0" y1="0" x2="-1.27" y2="-1.905" width="0.254" layer="94"/>
<wire x1="1.27" y1="-0.635" x2="0" y2="1.27" width="0.254" layer="94"/>
<wire x1="0" y1="1.27" x2="-1.27" y2="-0.635" width="0.254" layer="94"/>
<text x="-2.54" y="-5.08" size="1.778" layer="96" rot="R90">&gt;VALUE</text>
<pin name="+12V" x="0" y="-2.54" visible="off" length="short" direction="sup" rot="R90"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="+5V" urn="urn:adsk.eagle:component:26963/1" prefix="P+" library_version="1">
<description>&lt;b&gt;SUPPLY SYMBOL&lt;/b&gt;</description>
<gates>
<gate name="1" symbol="+5V" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="GND" urn="urn:adsk.eagle:component:26954/1" prefix="GND" library_version="1">
<description>&lt;b&gt;SUPPLY SYMBOL&lt;/b&gt;</description>
<gates>
<gate name="1" symbol="GND" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="+12V" urn="urn:adsk.eagle:component:26959/1" prefix="P+" library_version="1">
<description>&lt;b&gt;SUPPLY SYMBOL&lt;/b&gt;</description>
<gates>
<gate name="1" symbol="+12V" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
</libraries>
<attributes>
</attributes>
<variantdefs>
</variantdefs>
<classes>
<class number="0" name="default" width="0" drill="0">
</class>
</classes>
<parts>
<part name="K1" library="relay" library_urn="urn:adsk.eagle:library:339" deviceset="G5L" device="" package3d_urn="urn:adsk.eagle:package:24308/1"/>
<part name="K2" library="relay" library_urn="urn:adsk.eagle:library:339" deviceset="G5L" device="" package3d_urn="urn:adsk.eagle:package:24308/1"/>
<part name="K3" library="relay" library_urn="urn:adsk.eagle:library:339" deviceset="G5L" device="" package3d_urn="urn:adsk.eagle:package:24308/1"/>
<part name="P+1" library="supply1" library_urn="urn:adsk.eagle:library:371" deviceset="+5V" device=""/>
<part name="GND1" library="supply1" library_urn="urn:adsk.eagle:library:371" deviceset="GND" device=""/>
<part name="P+2" library="supply1" library_urn="urn:adsk.eagle:library:371" deviceset="+12V" device=""/>
<part name="K4" library="relay" library_urn="urn:adsk.eagle:library:339" deviceset="G5L" device="" package3d_urn="urn:adsk.eagle:package:24308/1"/>
<part name="K5" library="relay" library_urn="urn:adsk.eagle:library:339" deviceset="G5L" device="" package3d_urn="urn:adsk.eagle:package:24308/1"/>
<part name="K6" library="relay" library_urn="urn:adsk.eagle:library:339" deviceset="G5L" device="" package3d_urn="urn:adsk.eagle:package:24308/1"/>
<part name="K7" library="relay" library_urn="urn:adsk.eagle:library:339" deviceset="G5L" device="" package3d_urn="urn:adsk.eagle:package:24308/1"/>
</parts>
<sheets>
<sheet>
<plain>
</plain>
<instances>
<instance part="K1" gate="1" x="12.7" y="71.12" smashed="yes">
<attribute name="VALUE" x="13.97" y="74.041" size="1.778" layer="96"/>
<attribute name="PART" x="13.97" y="76.2" size="1.778" layer="95"/>
</instance>
<instance part="K1" gate="2" x="30.48" y="68.58" smashed="yes">
<attribute name="PART" x="33.02" y="68.58" size="1.778" layer="95"/>
</instance>
<instance part="K2" gate="1" x="63.5" y="71.12" smashed="yes">
<attribute name="VALUE" x="64.77" y="74.041" size="1.778" layer="96"/>
<attribute name="PART" x="64.77" y="76.2" size="1.778" layer="95"/>
</instance>
<instance part="K2" gate="2" x="81.28" y="68.58" smashed="yes">
<attribute name="PART" x="83.82" y="68.58" size="1.778" layer="95"/>
</instance>
<instance part="K3" gate="1" x="38.1" y="35.56" smashed="yes">
<attribute name="VALUE" x="39.37" y="38.481" size="1.778" layer="96"/>
<attribute name="PART" x="39.37" y="40.64" size="1.778" layer="95"/>
</instance>
<instance part="K3" gate="2" x="55.88" y="33.02" smashed="yes">
<attribute name="PART" x="58.42" y="33.02" size="1.778" layer="95"/>
</instance>
<instance part="P+1" gate="1" x="30.48" y="96.52" smashed="yes">
<attribute name="VALUE" x="27.94" y="91.44" size="1.778" layer="96" rot="R90"/>
</instance>
<instance part="GND1" gate="1" x="53.34" y="96.52" smashed="yes" rot="R180">
<attribute name="VALUE" x="55.88" y="99.06" size="1.778" layer="96" rot="R180"/>
</instance>
<instance part="P+2" gate="1" x="45.72" y="96.52" smashed="yes">
<attribute name="VALUE" x="43.18" y="91.44" size="1.778" layer="96" rot="R90"/>
</instance>
<instance part="K4" gate="1" x="38.1" y="5.08" smashed="yes">
<attribute name="VALUE" x="39.37" y="8.001" size="1.778" layer="96"/>
<attribute name="PART" x="39.37" y="10.16" size="1.778" layer="95"/>
</instance>
<instance part="K4" gate="2" x="55.88" y="2.54" smashed="yes">
<attribute name="PART" x="58.42" y="2.54" size="1.778" layer="95"/>
</instance>
<instance part="K5" gate="1" x="38.1" y="-22.86" smashed="yes">
<attribute name="VALUE" x="39.37" y="-19.939" size="1.778" layer="96"/>
<attribute name="PART" x="39.37" y="-17.78" size="1.778" layer="95"/>
</instance>
<instance part="K5" gate="2" x="55.88" y="-25.4" smashed="yes">
<attribute name="PART" x="58.42" y="-25.4" size="1.778" layer="95"/>
</instance>
<instance part="K6" gate="1" x="38.1" y="-53.34" smashed="yes">
<attribute name="VALUE" x="39.37" y="-50.419" size="1.778" layer="96"/>
<attribute name="PART" x="39.37" y="-48.26" size="1.778" layer="95"/>
</instance>
<instance part="K6" gate="2" x="55.88" y="-55.88" smashed="yes">
<attribute name="PART" x="58.42" y="-55.88" size="1.778" layer="95"/>
</instance>
<instance part="K7" gate="1" x="38.1" y="-83.82" smashed="yes">
<attribute name="VALUE" x="39.37" y="-80.899" size="1.778" layer="96"/>
<attribute name="PART" x="39.37" y="-78.74" size="1.778" layer="95"/>
</instance>
<instance part="K7" gate="2" x="55.88" y="-86.36" smashed="yes">
<attribute name="PART" x="58.42" y="-86.36" size="1.778" layer="95"/>
</instance>
</instances>
<busses>
</busses>
<nets>
<net name="+12V" class="0">
<segment>
<pinref part="K1" gate="2" pin="S"/>
<wire x1="25.4" y1="73.66" x2="22.86" y2="73.66" width="0.1524" layer="91"/>
<wire x1="22.86" y1="73.66" x2="22.86" y2="86.36" width="0.1524" layer="91"/>
<wire x1="22.86" y1="86.36" x2="45.72" y2="86.36" width="0.1524" layer="91"/>
<wire x1="45.72" y1="86.36" x2="88.9" y2="86.36" width="0.1524" layer="91"/>
<wire x1="88.9" y1="86.36" x2="88.9" y2="73.66" width="0.1524" layer="91"/>
<pinref part="K2" gate="2" pin="O"/>
<wire x1="88.9" y1="73.66" x2="86.36" y2="73.66" width="0.1524" layer="91"/>
<pinref part="P+2" gate="1" pin="+12V"/>
<wire x1="45.72" y1="93.98" x2="45.72" y2="86.36" width="0.1524" layer="91"/>
<junction x="45.72" y="86.36"/>
</segment>
</net>
<net name="GND" class="0">
<segment>
<pinref part="K1" gate="2" pin="O"/>
<wire x1="35.56" y1="73.66" x2="40.64" y2="73.66" width="0.1524" layer="91"/>
<wire x1="40.64" y1="73.66" x2="40.64" y2="83.82" width="0.1524" layer="91"/>
<pinref part="K2" gate="2" pin="S"/>
<wire x1="40.64" y1="83.82" x2="53.34" y2="83.82" width="0.1524" layer="91"/>
<wire x1="53.34" y1="83.82" x2="76.2" y2="83.82" width="0.1524" layer="91"/>
<wire x1="76.2" y1="83.82" x2="76.2" y2="73.66" width="0.1524" layer="91"/>
<pinref part="GND1" gate="1" pin="GND"/>
<wire x1="53.34" y1="93.98" x2="53.34" y2="83.82" width="0.1524" layer="91"/>
<junction x="53.34" y="83.82"/>
</segment>
</net>
<net name="N$1" class="0">
<segment>
<pinref part="K1" gate="2" pin="P"/>
<wire x1="30.48" y1="66.04" x2="30.48" y2="55.88" width="0.1524" layer="91"/>
<wire x1="30.48" y1="55.88" x2="50.8" y2="55.88" width="0.1524" layer="91"/>
<pinref part="K3" gate="2" pin="S"/>
<wire x1="50.8" y1="38.1" x2="50.8" y2="55.88" width="0.1524" layer="91"/>
<pinref part="K4" gate="2" pin="S"/>
<wire x1="50.8" y1="38.1" x2="50.8" y2="7.62" width="0.1524" layer="91"/>
<junction x="50.8" y="38.1"/>
<pinref part="K5" gate="2" pin="S"/>
<wire x1="50.8" y1="7.62" x2="50.8" y2="-20.32" width="0.1524" layer="91"/>
<junction x="50.8" y="7.62"/>
<wire x1="50.8" y1="-20.32" x2="50.8" y2="-50.8" width="0.1524" layer="91"/>
<junction x="50.8" y="-20.32"/>
<pinref part="K6" gate="2" pin="S"/>
<pinref part="K7" gate="2" pin="S"/>
<wire x1="50.8" y1="-50.8" x2="50.8" y2="-81.28" width="0.1524" layer="91"/>
<junction x="50.8" y="-50.8"/>
</segment>
</net>
<net name="N$2" class="0">
<segment>
<pinref part="K3" gate="2" pin="P"/>
<wire x1="55.88" y1="30.48" x2="55.88" y2="20.32" width="0.1524" layer="91"/>
<wire x1="55.88" y1="20.32" x2="116.84" y2="20.32" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$3" class="0">
<segment>
<pinref part="K2" gate="2" pin="P"/>
<wire x1="81.28" y1="66.04" x2="83.82" y2="66.04" width="0.1524" layer="91"/>
<wire x1="83.82" y1="66.04" x2="83.82" y2="25.4" width="0.1524" layer="91"/>
<wire x1="83.82" y1="25.4" x2="116.84" y2="25.4" width="0.1524" layer="91"/>
<wire x1="83.82" y1="25.4" x2="83.82" y2="-2.54" width="0.1524" layer="91"/>
<junction x="83.82" y="25.4"/>
<wire x1="83.82" y1="-2.54" x2="116.84" y2="-2.54" width="0.1524" layer="91"/>
<wire x1="83.82" y1="-2.54" x2="83.82" y2="-33.02" width="0.1524" layer="91"/>
<junction x="83.82" y="-2.54"/>
<wire x1="83.82" y1="-33.02" x2="119.38" y2="-33.02" width="0.1524" layer="91"/>
<wire x1="83.82" y1="-33.02" x2="83.82" y2="-60.96" width="0.1524" layer="91"/>
<junction x="83.82" y="-33.02"/>
<wire x1="83.82" y1="-60.96" x2="119.38" y2="-60.96" width="0.1524" layer="91"/>
<wire x1="83.82" y1="-60.96" x2="83.82" y2="-93.98" width="0.1524" layer="91"/>
<junction x="83.82" y="-60.96"/>
<wire x1="83.82" y1="-93.98" x2="121.92" y2="-93.98" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$4" class="0">
<segment>
<pinref part="K4" gate="2" pin="P"/>
<wire x1="55.88" y1="0" x2="55.88" y2="-7.62" width="0.1524" layer="91"/>
<wire x1="55.88" y1="-7.62" x2="119.38" y2="-7.62" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$5" class="0">
<segment>
<pinref part="K5" gate="2" pin="P"/>
<wire x1="55.88" y1="-27.94" x2="55.88" y2="-38.1" width="0.1524" layer="91"/>
<wire x1="55.88" y1="-38.1" x2="119.38" y2="-38.1" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$6" class="0">
<segment>
<pinref part="K6" gate="2" pin="P"/>
<wire x1="55.88" y1="-58.42" x2="55.88" y2="-66.04" width="0.1524" layer="91"/>
<wire x1="55.88" y1="-66.04" x2="119.38" y2="-66.04" width="0.1524" layer="91"/>
</segment>
</net>
<net name="N$7" class="0">
<segment>
<pinref part="K7" gate="2" pin="P"/>
<wire x1="55.88" y1="-88.9" x2="55.88" y2="-99.06" width="0.1524" layer="91"/>
<wire x1="55.88" y1="-99.06" x2="121.92" y2="-99.06" width="0.1524" layer="91"/>
</segment>
</net>
</nets>
</sheet>
</sheets>
</schematic>
</drawing>
<compatibility>
<note version="8.2" severity="warning">
Since Version 8.2, EAGLE supports online libraries. The ids
of those online libraries will not be understood (or retained)
with this version.
</note>
<note version="8.3" severity="warning">
Since Version 8.3, EAGLE supports URNs for individual library
assets (packages, symbols, and devices). The URNs of those assets
will not be understood (or retained) with this version.
</note>
<note version="8.3" severity="warning">
Since Version 8.3, EAGLE supports the association of 3D packages
with devices in libraries, schematics, and board files. Those 3D
packages will not be understood (or retained) with this version.
</note>
</compatibility>
</eagle>
