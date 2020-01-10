The Windows part of Intel(R) MPI Benchmarks 3.2 has the following
subdirectories:

  - .\IMB-EXT_VS_2005 (Microsoft* Visual Studio* 2005 project folder)
  - .\IMB-EXT_VS_2008 (Microsoft* Visual Studio* 2008 project folder)
  - .\IMB-IO_VS_2005  (Microsoft* Visual Studio* 2005 project folder)
  - .\IMB-IO_VS_2008  (Microsoft* Visual Studio* 2008 project folder)
  - .\IMB-MPI1_VS_2005 (Microsoft* Visual Studio* 2005 project folder)
  - .\IMB-MPI1_VS_2008 (Microsoft* Visual Studio* 2008 project folder)

Note that the Microsoft* Visual Studio* project folders IMB-EXT_VS_2005,
IMB-EXT_VS_2008, IMB-IO_VS_2005, IMB-IO_VS_2008, IMB-MPI1_VS_2005, and
IMB-MPI1_VS_2008 are applicable only to Microsoft* Windows*. Some
familiarity with Microsoft* Visual Studio* and the Windows Cluster at
hand is assumed. This ReadMe does not provide complete guidance.

The solutions are based on an install of Intel(R) MPI Library for Windows
(see the environment variable I_MPI_ROOT).

The internal folder structure for these Microsoft* Visual Studio* projects
look something like the following:
                 ...
    |
    +--\IMB-EXT_VS_2005
    |   |
    |   +--\x64
    |   |   |
    |   |   +--\Debug
    |   |   |
    |   |   +--\Release
    |   |
    |   +--IMB-EXT.rc
    |   |
    |   +--IMB-EXT.sln
    |   |
    |   +--IMB-EXT.vcproj
    |   |
    |   +--resource.h
    |
    +--\IMB-EXT_VS_2008
    |   |
    |   +--\x64
    |   |   |
    |   |   +--\Debug
    |   |   |
    |   |   +--\Release
    |   |
    |   +--IMB-EXT.rc
    |   |
    |   +--IMB-EXT.sln
    |   |
    |   +--IMB-EXT.vcproj
    |   |
    |   +--resource.h
    |
    +--\IMB-IO_VS_2005
    |   |
    |   +--\x64
    |   |   |
    |   |   +--\Debug
    |   |   |
    |   |   +--\Release
    |   |
    |   +--IMB-IO.rc
    |   |
    |   +--IMB-IO.sln
    |   |
    |   +--IMB-IO.vcproj
    |   |
    |   +--resource.h
    |
    +--\IMB-IO_VS_2008
    |   |
    |   +--\x64
    |   |   |
    |   |   +--\Debug
    |   |   |
    |   |   +--\Release
    |   |
    |   +--IMB-IO.rc
    |   |
    |   +--IMB-IO.sln
    |   |
    |   +--IMB-IO.vcproj
    |   |
    |   +--resource.h
    |
    +--\IMB-MPI1_VS_2005
    |   |
    |   +--\x64
    |   |   |
    |   |   +--\Debug
    |   |   |
    |   |   +--\Release
    |   |
    |   +--IMB-MPI1.rc
    |   |
    |   +--IMB-MPI1.sln
    |   |
    |   +--IMB-MPI1.vcproj
    |   |
    |   +--resource.h
    |
    +--\IMB-MPI1_VS_2008
        |
        +--\x64
        |   |
        |   +--\Debug
        |   |
        |   +--\Release
        |
        +--IMB-MPI1.rc
        |
        +--IMB-MPI1.sln
        |
        +--IMB-MPI1.vcproj
        |
        +--resource.h
 
When the Intel(R) MPI Benchmarks 3.2 is installed on a Microsoft* Windows*
system, one can simply click on the respective ".vcproj" project file and
use the Microsoft* Visual Studio* menu to run the associated benchmark
application.

Within Microsoft Windows Explorer, go to one of the folders IMB-EXT_VS_2005, 
IMB-EXT_VS_2008, IMB-IO_VS_2005, IMB-IO_VS_2008, IMB-MPI1_VS_2005, or 
IMB-MPI1_VS_2008 and click on the corresponding ".vcproj" file.

From the Visual Studio Project panel:
1) Change the "Solution Platforms" dialog box to "x64".
2) Change the "Solution Configurations" dialog box to "Release".
3) Check other settings in the usual style, e.g.
      C/C++->General 
         "Additional Include Directories", here set to "$(I_MPI_ROOT)\em64t\include".
      Linker->Input
         "Additional Dependencies", here "$(I_MPI_ROOT)\em64t\lib\impi.lib"
4) Use F7 or Build->Build Solution to create an executable

*Other brands and names may be claimed as the property of others. All
trademarks and registered trademarks referenced in this Intel(r) MPI
Benchmarks ReadMe.txt file are the property of their respective holders.
