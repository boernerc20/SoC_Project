
?
Command: %s
53*	vivadotcl2
place_designZ4-113h px� 

@Attempting to get a license for feature '%s' and/or device '%s'
308*common2
Implementation2	
xc7z020Z17-347h px� 
o
0Got license for feature '%s' and/or device '%s'
310*common2
Implementation2	
xc7z020Z17-349h px� 
H
Releasing license: %s
83*common2
ImplementationZ17-83h px� 
>
Running DRC with %s threads
24*drc2
8Z23-27h px� 
D
DRC finished with %s
79*	vivadotcl2

0 ErrorsZ4-198h px� 
e
BPlease refer to the DRC report (report_drc) for more information.
80*	vivadotclZ4-199h px� 
^
,Running DRC as a precondition to command %s
22*	vivadotcl2
place_designZ4-22h px� 
>
Running DRC with %s threads
24*drc2
8Z23-27h px� 
D
DRC finished with %s
79*	vivadotcl2

0 ErrorsZ4-198h px� 
e
BPlease refer to the DRC report (report_drc) for more information.
80*	vivadotclZ4-199h px� 
k
BMultithreading enabled for place_design using a maximum of %s CPUs12*	placeflow2
8Z30-611h px� 
C

Starting %s Task
103*constraints2
PlacerZ18-103h px� 
R

Phase %s%s
101*constraints2
1 2
Placer InitializationZ18-101h px� 
d

Phase %s%s
101*constraints2
1.1 2'
%Placer Initialization Netlist SortingZ18-101h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Netlist sorting complete. 2

00:00:002

00:00:002

2949.7462
0.0002
485542
62726Z17-722h px� 
`
%s*common2G
EPhase 1.1 Placer Initialization Netlist Sorting | Checksum: 9477fc6b
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:00:00 ; elapsed = 00:00:00 . Memory (MB): peak = 2949.746 ; gain = 0.000 ; free physical = 48554 ; free virtual = 62726h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Netlist sorting complete. 2

00:00:002

00:00:002

2949.7462
0.0002
485542
62726Z17-722h px� 
q

Phase %s%s
101*constraints2
1.2 24
2IO Placement/ Clock Placement/ Build Placer DeviceZ18-101h px� 
E
%Done setting XDC timing constraints.
35*timingZ38-35h px� 
m
%s*common2T
RPhase 1.2 IO Placement/ Clock Placement/ Build Placer Device | Checksum: 54e9c054
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:00:00.34 ; elapsed = 00:00:00.21 . Memory (MB): peak = 2949.746 ; gain = 0.000 ; free physical = 48554 ; free virtual = 62726h px� 
Y

Phase %s%s
101*constraints2
1.3 2
Build Placer Netlist ModelZ18-101h px� 
s
PTiming driven mode will be turned off because no critical terminals were found.
1337*placeZ30-2953h px� 
U
%s*common2<
:Phase 1.3 Build Placer Netlist Model | Checksum: cdc5e67d
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:00:00.66 ; elapsed = 00:00:00.27 . Memory (MB): peak = 2949.746 ; gain = 0.000 ; free physical = 48554 ; free virtual = 62727h px� 
V

Phase %s%s
101*constraints2
1.4 2
Constrain Clocks/MacrosZ18-101h px� 
R
%s*common29
7Phase 1.4 Constrain Clocks/Macros | Checksum: cdc5e67d
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:00:00.67 ; elapsed = 00:00:00.27 . Memory (MB): peak = 2949.746 ; gain = 0.000 ; free physical = 48554 ; free virtual = 62728h px� 
N
%s*common25
3Phase 1 Placer Initialization | Checksum: cdc5e67d
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:00:00.67 ; elapsed = 00:00:00.27 . Memory (MB): peak = 2949.746 ; gain = 0.000 ; free physical = 48554 ; free virtual = 62728h px� 
T

Phase %s%s
101*constraints2
2 2
Final Placement CleanupZ18-101h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Netlist sorting complete. 2

00:00:002

00:00:002

2949.7462
0.0002
485542
62728Z17-722h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:00:00.67 ; elapsed = 00:00:00.27 . Memory (MB): peak = 2949.746 ; gain = 0.000 ; free physical = 48554 ; free virtual = 62728h px� 
�
aNo place-able instance is found; design doesn't contain any instance or all instances are placed
5*	placeflowZ30-281h px� 
C
%s*common2*
(Ending Placer Task | Checksum: 54e9c054
h px� 
�

%s
*constraints2�
�Time (s): cpu = 00:00:00.68 ; elapsed = 00:00:00.28 . Memory (MB): peak = 2949.746 ; gain = 0.000 ; free physical = 48554 ; free virtual = 62728h px� 
~
G%s Infos, %s Warnings, %s Critical Warnings and %s Errors encountered.
28*	vivadotcl2
442
12
02
0Z4-41h px� 
L
%s completed successfully
29*	vivadotcl2
place_designZ4-42h px� 
X
%s4*runtcl2<
:Executing : report_io -file base_ps_wrapper_io_placed.rpt
h px� 
�
�report_io: Time (s): cpu = 00:00:00.07 ; elapsed = 00:00:00.12 . Memory (MB): peak = 2949.746 ; gain = 0.000 ; free physical = 48546 ; free virtual = 62720
*commonh px� 
�
%s4*runtcl2x
vExecuting : report_utilization -file base_ps_wrapper_utilization_placed.rpt -pb base_ps_wrapper_utilization_placed.pb
h px� 
u
%s4*runtcl2Y
WExecuting : report_control_sets -verbose -file base_ps_wrapper_control_sets_placed.rpt
h px� 
�
�report_control_sets: Time (s): cpu = 00:00:00 ; elapsed = 00:00:00.06 . Memory (MB): peak = 2949.746 ; gain = 0.000 ; free physical = 48545 ; free virtual = 62720
*commonh px� 
H
&Writing timing data to binary archive.266*timingZ38-480h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Write ShapeDB Complete: 2
00:00:00.012

00:00:002

2949.7462
0.0002
485452
62720Z17-722h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Wrote PlaceDB: 2
00:00:00.032
00:00:00.012

2949.7462
0.0002
485452
62720Z17-722h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Wrote PulsedLatchDB: 2

00:00:002

00:00:002

2949.7462
0.0002
485452
62720Z17-722h px� 
=
Writing XDEF routing.
211*designutilsZ20-211h px� 
J
#Writing XDEF routing logical nets.
209*designutilsZ20-209h px� 
J
#Writing XDEF routing special nets.
210*designutilsZ20-210h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Wrote RouteStorage: 2
00:00:00.052
00:00:00.012

2949.7462
0.0002
485452
62720Z17-722h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Wrote Netlist Cache: 2

00:00:002
00:00:00.012

2949.7462
0.0002
485452
62720Z17-722h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Wrote Device Cache: 2
00:00:00.012

00:00:002

2949.7462
0.0002
485442
62720Z17-722h px� 
�
r%sTime (s): cpu = %s ; elapsed = %s . Memory (MB): peak = %s ; gain = %s ; free physical = %s ; free virtual = %s
480*common2
Write Physdb Complete: 2
00:00:00.092
00:00:00.032

2949.7462
0.0002
485442
62720Z17-722h px� 
�
 The %s '%s' has been generated.
621*common2

checkpoint2M
K/home/boernerc20/zc702_soc/zc702_soc.runs/impl_1/base_ps_wrapper_placed.dcpZ17-1381h px� 


End Record