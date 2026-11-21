// Integration of the actual app's atomic writer; uses only synthetic saves in
// the explicitly supplied scratch directory. Never pass a game save directory.
#define wWinMain SuzerainUnusedGuiEntry
#include "../src/main.cpp"
#undef wWinMain
#include <iostream>
#include <functional>
#include <thread>

int main(int argc,char** argv){
    if(argc!=2){std::cerr<<"Usage: test_app_io <new-scratch-directory>\n";return 2;}
    size_t checks=0;
    auto check=[&](bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);};
    auto rejects=[&](const std::function<void()>& operation,const char* text){bool rejected=false;try{operation();}catch(const std::exception&){rejected=true;}check(rejected,text);};
    const fs::path root=fs::absolute(fs::u8path(argv[1]));
    try{
        if(fs::exists(root))throw std::runtime_error("Scratch directory must be new");
        if(gameRunning())throw std::runtime_error("Game is running; synthetic writer tests not started");
        fs::create_directories(root);
        auto save=[](const std::string& story,const std::string& prefix,int value){
            return "{\"currentStoryPack\":\""+story+"\",\"version\":\"test\",\"campaignName\":\"synthetic\",\"turnNo\":1,\"variables\":\"Variable={[\\\""+prefix+"Budget\\\"]="+std::to_string(value)+",[\\\"OtherMod.Keep\\\"]=17};\"}";
        };
        auto write=[](const fs::path& path,const std::string& data){std::ofstream out(path,std::ios::binary);out<<data;if(!out)throw std::runtime_error("Fixture write failed");};
        const auto riziaOld=save("StoryPack_Rizia","RiziaDLC.",5),riziaNew=save("StoryPack_Rizia","RiziaDLC.",6);
        const auto mainOld=save("StoryPack_Main","BaseGame.",7),mainNew=save("StoryPack_Main","BaseGame.",8);
        const auto riziaPath=root/"rizia.json",mainPath=root/"main.json";
        write(riziaPath,riziaOld);write(mainPath,mainOld);
        {
            SaveLock held(mainPath);
            bool busy = false;
            std::thread competing([&] {
                try { atomicSave(mainPath,mainNew,&mainOld); }
                catch (const WinFailure&) { busy = true; }
            });
            competing.join();
            check(busy && readFile(mainPath)==mainOld && !fs::exists(root/".rizia-backups"),
                  "Concurrent save was not blocked before backup/write");
        }
        const auto practiceCopy=root/"practice.json";
        check(atomicSave(practiceCopy,mainNew,nullptr,true).empty()&&readFile(practiceCopy)==mainNew,"Practice copy was not created");
        rejects([&]{atomicSave(mainPath,mainNew,nullptr,true);},"Practice save overwrote an existing file");
        check(readFile(mainPath)==mainOld&&!fs::exists(root/".rizia-backups"),"Rejected practice save changed the destination");
        rejects([&]{atomicSave(mainPath,riziaNew,nullptr);},"Cross-story overwrite accepted");
        check(readFile(mainPath)==mainOld&&!fs::exists(root/".rizia-backups"),"Rejected cross-story overwrite changed files");
        rejects([&]{atomicSave(riziaPath,mainNew,&riziaOld);},"Cross-story overwrite with expected bytes accepted");
        check(readFile(riziaPath)==riziaOld,"Rizia changed on mismatch");
        const auto unknown=save("StoryPack_Unknown","BaseGame.",8);
        rejects([&]{atomicSave(root/"unknown.json",unknown,nullptr);},"Unknown output story accepted");
        check(!fs::exists(root/"unknown.json"),"Unknown output created file");
        std::string stale=mainOld+" ";
        rejects([&]{atomicSave(mainPath,mainNew,&stale);},"Stale opened bytes accepted");
        check(readFile(mainPath)==mainOld,"Stale rejection changed save");
        const auto mainBackup=atomicSave(mainPath,mainNew,&mainOld);
        check(readFile(mainPath)==mainNew&&readFile(mainBackup)==mainOld,"Main save/backup not byte-exact");
        const auto riziaBackup=atomicSave(riziaPath,riziaNew,&riziaOld);
        check(readFile(riziaPath)==riziaNew&&readFile(riziaBackup)==riziaOld,"Rizia save/backup not byte-exact");
        rejects([&]{atomicSave(mainPath,readFile(riziaBackup),&mainNew);},"Foreign backup restored over Main");
        check(readFile(mainPath)==mainNew,"Foreign restore changed target");
        const auto restoreBackup=atomicSave(mainPath,readFile(mainBackup),&mainNew);
        check(readFile(mainPath)==mainOld&&readFile(restoreBackup)==mainNew,"Restore or pre-restore backup not exact");
        check(readFile(mainBackup)==mainOld,"Restore mutated source backup");
        auto external=mainOld+"\n";write(mainPath,external);
        rejects([&]{atomicSave(mainPath,mainNew,&mainOld);},"Externally changed file accepted");
        check(readFile(mainPath)==external,"Conflict overwrote external change");
        const auto newPath=root/"new-main.json";
        check(atomicSave(newPath,mainNew,nullptr).empty()&&readFile(newPath)==mainNew,"Save As new path failed");
        bool temporary=false;for(const auto& entry:fs::directory_iterator(root))if(entry.path().extension()==".tmp")temporary=true;
        check(!temporary,"Temporary write file left behind");
        // Exercise patch/CSV writer on scratch only; production dialogs remain manual QA.
        const auto mainPatch=rizia::serializePatch({{"BaseGame.Budget","9"}},"StoryPack_Main");
        writePatch(root/"main.patch.txt",mainPatch);
        check(rizia::parsePatch(readFile(root/"main.patch.txt"),"StoryPack_Main").at("BaseGame.Budget")=="9","Patch writer roundtrip failed");
        const std::string csv="\xEF\xBB\xBF" "Campaign,Field,Original,Current\r\nStoryPack_Main,BaseGame.Budget,7,8\r\n";
        writePatch(root/"fields.csv",csv);
        check(readFile(root/"fields.csv")==csv,"CSV bytes changed by writer");
        const auto gameFolder=root/"sample-game";
        const auto gameExe=gameFolder/"Suzerain.exe";
        const auto host=gameFolder/"BepInEx";
        const auto payload=root/live_setup::pluginName;
        const auto installed=host/"plugins"/"SuzerainSaveStudio"/live_setup::pluginName;
        fs::create_directories(gameFolder);
        write(payload,"synthetic plugin bytes");write(gameExe,"synthetic exe");
        rejects([&]{live_setup::install(payload,gameExe);},"Setup accepted missing mod host");
        check(!fs::exists(host/"plugins"),"Setup changed an unsupported installation");
        fs::create_directories(host/"core");fs::create_directories(host/"interop");
        write(gameFolder/"GameAssembly.dll","fixture");write(host/"core"/"BepInEx.Unity.IL2CPP.dll","fixture");
        write(host/"interop"/"Assembly-CSharp.dll","fixture");
        check(live_setup::install(payload,gameExe)&&readFile(installed)==readFile(payload),"Plugin installation");
        check(!live_setup::install(payload,gameExe),"Setup should recognize an identical installation");
        write(installed,"older user-controlled plugin");
        rejects([&]{live_setup::install(payload,gameExe);},"Setup overwrote a different plugin");
        check(readFile(installed)=="older user-controlled plugin","Setup changed the previous plugin");
        std::cout<<"PASS: "<<checks<<" synthetic app IO checks; fixtures retained at "<<root.u8string()<<'\n';
        return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n';return 1;}
}
