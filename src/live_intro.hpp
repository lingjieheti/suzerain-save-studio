#pragma once

namespace live_setup {
struct IntroCopy { const wchar_t *title,*setup,*missing,*action; };

inline const IntroCopy& introCopy(){
    static const IntroCopy copy[]={
        {L"在游戏中编辑资源",L"读取战役后，按 Ctrl + F8 打开实时面板。\n\n首次安装前，请先通过 BepInEx 6 IL2CPP 启动一次游戏。",L"已安装插件时，读取战役后按 Ctrl + F8 即可打开实时面板。\n\n首次安装请解压完整实时版 ZIP，将配套插件 DLL 与本程序放在同一文件夹，再打开实时模式。",L"选择 Suzerain.exe 并安装插件"},
        {L"在遊戲中編輯資源",L"載入戰役後，按 Ctrl + F8 開啟即時面板。\n\n首次安裝前，請先透過 BepInEx 6 IL2CPP 啟動一次遊戲。",L"已安裝外掛時，載入戰役後按 Ctrl + F8 即可開啟即時面板。\n\n首次安裝請解壓縮完整即時版 ZIP，將隨附的外掛 DLL 與本程式放在同一個資料夾，再開啟即時模式。",L"選擇 Suzerain.exe 並安裝外掛"},
        {L"Edit resources in game",L"Load a campaign and press Ctrl + F8 to open the live panel.\n\nBefore installing the plugin, launch the game once with BepInEx 6 IL2CPP.",L"If the plugin is installed, load a campaign and press Ctrl + F8.\n\nFor a first installation, extract the complete Live ZIP, keep its plugin DLL beside this application, and reopen Live mode.",L"Choose Suzerain.exe and install the plugin"},
        {L"ゲーム内で資源を編集",L"キャンペーンを読み込み、Ctrl + F8 でパネルを開きます。\n\n初回インストール前に BepInEx 6 IL2CPP で一度ゲームを起動してください。",L"導入済みの場合は、キャンペーン内で Ctrl + F8 を押してください。\n\n初回は完全版の Live ZIP を展開し、同梱の DLL を本アプリと同じフォルダーに置いてリアルタイムモードを開き直してください。",L"Suzerain.exe を選んでプラグインを導入"},
        {L"게임에서 자원 편집",L"캠페인을 불러온 뒤 Ctrl + F8을 눌러 패널을 여세요.\n\n처음 설치하기 전에 BepInEx 6 IL2CPP로 게임을 한 번 실행하세요.",L"플러그인이 설치되어 있다면 캠페인에서 Ctrl + F8을 누르세요.\n\n처음 설치할 때는 전체 Live ZIP의 압축을 풀고 플러그인 DLL을 이 앱과 같은 폴더에 둔 뒤 실시간 모드를 다시 여세요.",L"Suzerain.exe 선택 후 플러그인 설치"},
        {L"Ressourcen im Spiel bearbeiten",L"Eine Kampagne laden und mit Ctrl + F8 das Panel öffnen.\n\nVor der ersten Plugin-Installation das Spiel einmal mit BepInEx 6 IL2CPP starten.",L"Bei installiertem Plugin eine Kampagne laden und Ctrl + F8 drücken.\n\nFür die Erstinstallation die vollständige Live-ZIP entpacken, die Plugin-DLL neben dieser Anwendung ablegen und den Live-Modus erneut öffnen.",L"Suzerain.exe wählen und Plugin installieren"},
        {L"Modifier les ressources en jeu",L"Chargez une campagne et appuyez sur Ctrl + F8 pour ouvrir le panneau.\n\nAvant la première installation, lancez le jeu une fois avec BepInEx 6 IL2CPP.",L"Si le plugin est installé, chargez une campagne et appuyez sur Ctrl + F8.\n\nSinon, extrayez le ZIP Live complet, placez sa DLL à côté de cette application et rouvrez le mode en direct.",L"Choisir Suzerain.exe et installer le plugin"},
        {L"Editar recursos en el juego",L"Cargue una campaña y pulse Ctrl + F8 para abrir el panel.\n\nAntes de instalar el complemento, inicie el juego una vez con BepInEx 6 IL2CPP.",L"Si ya está instalado, cargue una campaña y pulse Ctrl + F8.\n\nPara instalarlo, extraiga el ZIP Live completo, coloque la DLL junto a esta aplicación y vuelva a abrir el modo en vivo.",L"Elegir Suzerain.exe e instalar el complemento"},
        {L"Editar recursos no jogo",L"Carregue uma campanha e pressione Ctrl + F8 para abrir o painel.\n\nAntes de instalar o plugin, execute o jogo uma vez com o BepInEx 6 IL2CPP.",L"Se o plugin já estiver instalado, carregue uma campanha e pressione Ctrl + F8.\n\nPara instalar, extraia o ZIP Live completo, coloque a DLL ao lado deste aplicativo e abra o modo ao vivo novamente.",L"Selecionar Suzerain.exe e instalar o plugin"},
        {L"Редактирование ресурсов в игре",L"Загрузите кампанию и нажмите Ctrl + F8, чтобы открыть панель.\n\nПеред первой установкой один раз запустите игру с BepInEx 6 IL2CPP.",L"Если плагин уже установлен, загрузите кампанию и нажмите Ctrl + F8.\n\nДля установки распакуйте полный ZIP-архив Live, поместите DLL рядом с приложением и снова откройте режим «В игре».",L"Выбрать Suzerain.exe и установить плагин"},
        {L"Oyun içi kaynakları düzenle",L"Bir senaryo yükleyip Ctrl + F8 ile paneli açın.\n\nEklentiyi ilk kez kurmadan önce oyunu BepInEx 6 IL2CPP ile bir kez başlatın.",L"Eklenti kuruluysa bir senaryo yükleyip Ctrl + F8 tuşlarına basın.\n\nİlk kurulum için tam Live ZIP dosyasını açın, eklenti DLL dosyasını bu uygulamanın yanına koyun ve Canlı modu yeniden açın.",L"Suzerain.exe seç ve eklentiyi kur"}
    };
    return copy[i18n::language];
}

inline bool openIntro(HWND owner,bool canInstall){
    const auto& copy=introCopy();
    const auto title=tr(L"实时模式");
    TASKDIALOG_BUTTON command{chooseGameAction,copy.action};
    TASKDIALOGCONFIG config{};
    config.cbSize=sizeof(config);
    config.hwndParent=owner;
    config.hInstance=GetModuleHandleW(nullptr);
    config.dwFlags=TDF_ALLOW_DIALOG_CANCELLATION|TDF_POSITION_RELATIVE_TO_WINDOW;
    config.pszWindowTitle=title.c_str();
    config.pszMainInstruction=copy.title;
    config.pszContent=canInstall?copy.setup:copy.missing;
    config.pszMainIcon=MAKEINTRESOURCEW(101);
    config.cxWidth=285;
    if(canInstall){
        config.dwFlags|=TDF_USE_COMMAND_LINKS;
        config.cButtons=1;config.pButtons=&command;config.nDefaultButton=IDCANCEL;
        config.dwCommonButtons=TDCBF_CANCEL_BUTTON;
    }else config.dwCommonButtons=TDCBF_CLOSE_BUTTON;
    int chosen=IDCANCEL;
    if(SUCCEEDED(TaskDialogIndirect(&config,&chosen,nullptr,nullptr)))return canInstall&&chosen==chooseGameAction;
    // Common-controls v6 is declared in the manifest. Keep a usable system fallback.
    const auto fallback=std::wstring(config.pszContent)+(canInstall?L"\n\n"+std::wstring(command.pszButtonText):L"");
    return MessageBoxW(owner,fallback.c_str(),title.c_str(),(canInstall?MB_YESNO|MB_DEFBUTTON2:MB_OK)|MB_ICONINFORMATION)==IDYES&&canInstall;
}
}
