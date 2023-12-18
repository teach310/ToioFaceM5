# ToioFace M5

A product that treats the M5AtomS3 mounted on ATOM Mate for toio as a BLE Peripheral.

## Bluetooth LE

### Distance Characteristic

|Property|Value||
|:--|:--|:--|
|Service UUID |`0B21C05A-44C2-47CC-BFEF-4F7165C33908`|Custom UUID|
|Characteristic UUID |`29C2D1B2-944A-4FBA-AFCD-133E09532556`|Custom UUID|
|Properties|Read, Notify|

|Data Pos|Data Type|Description|
|:--|:--|:--|
|0|UInt16|distance (mm)|

### Expression Characteristic

|Property|Value||
|:--|:--|:--|
|Characteristic UUID |`29C2D1B2-944A-4FBA-AFCD-133E09532557`|Custom UUID|
|Properties|Read, Write||

|Data Pos|Data Type|Description|Example|
|:--|:--|:--|:--|
|0|UInt8|表情ID|0x00 (Happy)|