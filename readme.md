# 《植物大战僵尸》小游戏的破解工具(用于实现在游戏中作弊)  
## 功能  
1.修改阳光：在文本框中输入想要修改的数值，点击"修改阳光"按钮即可  
2.修改金币：在文本框中输入想要修改的数值，点击"修改金币"按钮即可  
3.清除所有僵尸：点击"清除所有僵尸"按钮  
4.无CD模式：勾选"无CD模式"即可取消植物的恢复时间  
5.吸附游戏窗口：勾选后修改器界面会贴在游戏窗口左侧  
6.重新定位游戏：对于先打开修改器，后打开游戏会导致修改器找不到游戏的进程，此时可以点击"重新定位游戏"修改器即可找到游戏进程  
注意：   修改阳光时请不要输入超过6位数的阳光值，否则修改器会关闭（阳光值不要大于999999）  
        勾选无CD模式后，下一关如果又回到了有cd的状态，请重新勾选  
## 简介  
  2017年左右，刚学完C++的我自学了WindowsApi和汇编语言。后来无意中在我同学的U盘中发现了这款名叫“植物大战僵尸”的小游戏，便尝试着结合自学的知识制作一个关于这块小游戏的作弊工具。在制作初期还是遇到了很多困难，但是网上相应的资料也有不少，因此最终克服了重重困难制作了这个工具。  
## 游戏版本  
这款工具只支持"植物大战僵尸中文版"的游戏，游戏的config.ini是:  
[info]  
id=100107922  
version=1.0.0.1001  
name=植物大战僵尸  
exec=PlantsVsZombies.exe  
如果版本不对应的话，这个工具是无法使用的!  
