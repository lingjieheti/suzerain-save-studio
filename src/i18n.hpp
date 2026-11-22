#pragma once
#include <windows.h>
#include <shlobj.h>
#include <array>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include "field_translations.hpp"
#include "field_labels_west.hpp"
#include "field_labels_east.hpp"

namespace i18n {
inline int language=0;
inline constexpr const wchar_t* names[11]={L"简体中文",L"繁體中文",L"English",L"日本語",L"한국어",L"Deutsch",L"Français",L"Español",L"Português (Brasil)",L"Русский",L"Türkçe"};
inline void setLanguage(int value){language=value>=0&&value<11?value:2;}
inline std::wstring preferencesPath(){
    PWSTR base=nullptr;if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&base)))return {};
    std::wstring directory=std::wstring(base)+L"\\SuzerainSaveStudio";CoTaskMemFree(base);
    if(!CreateDirectoryW(directory.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return {};
    return directory+L"\\settings.ini";
}
inline void loadPreferences(){auto path=preferencesPath();if(!path.empty())setLanguage((int)GetPrivateProfileIntW(L"UI",L"Language",0,path.c_str()));}
inline void savePreferences(){auto path=preferencesPath();if(!path.empty())WritePrivateProfileStringW(L"UI",L"Language",std::to_wstring(language).c_str(),path.c_str());}
inline std::wstring traditional(const std::wstring& s){
    if(s.empty())return s;int n=LCMapStringEx(L"zh-TW",LCMAP_TRADITIONAL_CHINESE,s.data(),(int)s.size(),nullptr,0,nullptr,nullptr,0);
    if(!n)return s;std::wstring out(n,L'\0');if(!LCMapStringEx(L"zh-TW",LCMAP_TRADITIONAL_CHINESE,s.data(),(int)s.size(),out.data(),n,nullptr,nullptr,0))return s;return out;
}
struct Entry {std::vector<std::wstring> text;};
inline const std::map<std::wstring,Entry>& dictionary(){
    static const auto rows=[](){std::map<std::wstring,Entry> out;
        const wchar_t* data[]={
// Chinese, English, Japanese, Korean, German, French, Spanish, Brazilian Portuguese, Russian, Turkish.
L"请输入非负整数。|Enter a nonnegative integer.|0 以上の整数を入力してください。|0 이상의 정수를 입력하세요.|Gib eine ganze Zahl ab 0 ein.|Saisissez un entier supérieur ou égal à zéro.|Introduce un número entero mayor o igual que cero.|Digite um número inteiro maior ou igual a zero.|Введите целое число не меньше нуля.|Sıfır veya pozitif bir tam sayı girin.",
L"修改方案没有处理完成。|Patch operation failed.|変更プランを処理できませんでした。|수정안을 처리하지 못했습니다.|Der Änderungsplan konnte nicht verarbeitet werden.|Impossible de traiter le fichier de modifications.|No se pudo procesar el archivo de cambios.|Não foi possível processar o arquivo de alterações.|Не удалось обработать файл изменений.|Değişiklik dosyası işlenemedi.",
L"当前存档中找不到这个可编辑字段。|Unknown, foreign-story or non-editable field|このセーブに該当する編集可能な項目がありません。|현재 세이브에 해당하는 편집 가능 필드가 없습니다.|Dieses Feld ist in diesem Spielstand nicht als bearbeitbares Feld vorhanden.|Ce champ est absent de cette sauvegarde ou ne peut pas être modifié.|Este campo no existe en esta partida o no se puede editar.|Este campo não existe neste arquivo salvo ou não pode ser editado.|В этом сохранении нет такого поля, доступного для редактирования.|Bu kayıtta bu adla düzenlenebilen bir alan yok.",
L"这个字段只接受 true 或 false。|Boolean fields accept only true or false|この項目には true または false を入力してください。|이 필드에는 true 또는 false만 입력할 수 있습니다.|Dieses Feld erlaubt nur true oder false.|Ce champ accepte uniquement true ou false.|Este campo solo admite true o false.|Este campo só aceita true ou false.|Допустимые значения этого поля — true и false.|Bu alana yalnızca true veya false girilebilir.",
L"输入的数值太长。|Replacement number is too long|入力した数値が長すぎます。|입력한 숫자가 너무 깁니다.|Die eingegebene Zahl ist zu lang.|Le nombre saisi est trop long.|El número introducido es demasiado largo.|O número digitado é longo demais.|Введённое число слишком длинное.|Girilen sayı çok uzun.",
L"请输入有限的十进制数。|Enter a finite decimal number, not text or an expression|有限の十進数を入力してください。|유한한 십진수를 입력하세요.|Gib eine endliche Dezimalzahl ein.|Saisissez un nombre décimal fini.|Introduce un número decimal finito.|Digite um número decimal finito.|Введите конечное десятичное число.|Sonlu bir ondalık sayı girin.",
L"数值的绝对值超过了上限 2147483647。|Number exceeds the allowed magnitude 2147483647|数値の絶対値が上限の 2147483647 を超えています。|숫자의 절댓값이 상한인 2147483647을 초과합니다.|Der Betrag der Zahl überschreitet 2147483647.|La valeur absolue du nombre dépasse 2147483647.|El valor absoluto del número supera 2147483647.|O valor absoluto do número excede 2147483647.|Число по модулю превышает 2147483647.|Sayının mutlak değeri 2147483647 sınırını aşıyor.",
L"请输入 -2147483647 到 2147483647 之间的有限数值。|Number must be finite and within -2147483647 to 2147483647|−2147483647 以上 2147483647 以下の有限の数値を入力してください。|−2147483647 이상 2147483647 이하의 유한한 숫자를 입력하세요.|Gib eine endliche Zahl zwischen −2147483647 und 2147483647 ein.|Saisissez un nombre fini compris entre −2147483647 et 2147483647.|Introduce un número finito entre −2147483647 y 2147483647.|Digite um número finito entre −2147483647 e 2147483647.|Введите конечное число в диапазоне от −2147483647 до 2147483647.|−2147483647 ile 2147483647 arasında sonlu bir sayı girin.",
L"目标文件已存在。请选择其他文件名。|The destination file already exists. Choose a different filename.|保存先のファイルは既に存在します。別のファイル名を選んでください。|대상 파일이 이미 있습니다. 다른 파일 이름을 선택하세요.|Die Zieldatei existiert bereits. Wähle einen anderen Dateinamen.|Le fichier de destination existe déjà. Choisissez un autre nom.|El archivo de destino ya existe. Elige otro nombre.|O arquivo de destino já existe. Escolha outro nome.|Файл с таким именем уже существует. Выберите другое имя.|Hedef dosya zaten var. Başka bir dosya adı seçin.",
L"实时模式|Live mode|リアルタイム|실시간 모드|Live-Modus|Mode en direct|Modo en vivo|Modo ao vivo|В игре|Canlı mod",
L"选择存档|Select save|セーブ選択|저장 선택|Spielstand wählen|Choisir une sauvegarde|Elegir partida|Escolher jogo salvo|Выбрать сохранение|Kayıt seç",
L"内置练习示例|Practice saves|練習用サンプル|연습용 예제|Übungsbeispiele|Exemples intégrés|Ejemplos de práctica|Exemplos de prática|Учебные примеры|Alıştırma örnekleri",
L"可体验编辑功能，无法在游戏中载入。|Try editing a sample. Samples cannot be loaded in the game.|編集を試せます。ゲームでは読み込めません。|편집을 연습할 수 있습니다. 게임에서는 불러올 수 없습니다.|Zum Ausprobieren. Im Spiel nicht ladbar.|Pour tester l’éditeur. Inutilisables dans le jeu.|Para probar el editor. No se cargan en el juego.|Teste o editor. Os exemplos não abrem no jogo.|Для знакомства с редактором. В игре не загружаются.|Düzenleyiciyi deneyin. Örnekler oyunda açılamaz.",
L"索德兰示例|Sordland demo|ソードランドの例|소르들란트 예제|Sordland-Demo|Démo Sordland|Demo de Sordland|Demo de Sordland|Пример Сордланда|Sordland örneği",
L"里奇亚示例|Rizia demo|リジアの例|리지아 예제|Rizia-Demo|Démo Rizia|Demo de Rizia|Demo de Rizia|Пример Ризии|Rizia örneği",
L"示例只能另存为新文件。|Choose a new filename to save a practice copy.|サンプルは新しいファイル名で保存してください。|예제는 새 파일 이름으로 저장하세요.|Das Beispiel unter einem neuen Dateinamen speichern.|Enregistrez l’exemple sous un nouveau nom.|Guarde el ejemplo con un nombre nuevo.|Salve o exemplo com um novo nome.|Сохраните пример под новым именем.|Örneği yeni bir dosya adıyla kaydedin.",
L"示例副本已保存。|Practice copy saved.|サンプルのコピーを保存しました。|예제 사본을 저장했습니다.|Beispielkopie gespeichert.|Copie de l’exemple enregistrée.|Copia del ejemplo guardada.|Cópia do exemplo salva.|Копия примера сохранена.|Örnek kopyası kaydedildi.",
L"无法读取内置示例。|Could not load the practice save.|サンプルを読み込めません。|예제를 불러올 수 없습니다.|Beispiel konnte nicht geladen werden.|Impossible de charger l’exemple.|No se pudo abrir el ejemplo.|Não foi possível abrir o exemplo.|Не удалось загрузить пример.|Örnek açılamadı.",
L"手动打开|Open file|ファイルを開く|파일 열기|Datei öffnen|Ouvrir un fichier|Abrir archivo|Abrir arquivo|Открыть файл|Dosya aç",
L"打开存档|Open save|セーブを開く|저장 열기|Spielstand öffnen|Ouvrir une sauvegarde|Abrir partida|Abrir jogo salvo|Открыть сохранение|Kayıt aç",
L"文件夹|Folder|フォルダー|폴더|Ordner|Dossier|Carpeta|Pasta|Папка|Klasör",
L"重新载入|Reload|再読込|다시 불러오기|Neu laden|Recharger|Recargar|Recarregar|Перезагрузить|Yeniden yükle",
L"刷新|Refresh|更新|새로 고침|Aktualisieren|Actualiser|Actualizar|Atualizar|Обновить|Yenile",
L"刷新列表|Refresh|更新|새로 고침|Aktualisieren|Actualiser|Actualizar|Atualizar|Обновить|Yenile",
L"添加文件夹|Add folder|フォルダー追加|폴더 추가|Ordner +|Ajouter dossier|Añadir carpeta|Adicionar pasta|Папка +|Klasör ekle",
L"打开所选|Open selected|選択を開く|선택 열기|Auswahl öffnen|Ouvrir la sélection|Abrir selección|Abrir seleção|Открыть выбранное|Seçileni aç",
L"取消|Cancel|キャンセル|취소|Abbrechen|Annuler|Cancelar|Cancelar|Отмена|İptal",
L"文件名|File name|ファイル名|파일 이름|Dateiname|Nom du fichier|Archivo|Arquivo|Имя файла|Dosya adı",
L"战役名称|Campaign name|キャンペーン名|캠페인 이름|Kampagnenname|Nom de campagne|Nombre de campaña|Nome da campanha|Имя кампании|Senaryo adı",
L"存档详情|Save details|セーブ詳細|저장 세부 정보|Spielstanddetails|Détails de la sauvegarde|Detalles de la partida|Detalhes do jogo salvo|Сведения о сохранении|Kayıt ayrıntıları",
L"所有战役|All campaigns|全キャンペーン|모든 캠페인|Alle Kampagnen|Toutes les campagnes|Todas las campañas|Todas as campanhas|Все кампании|Tüm senaryolar",
L"回合|Turn|ターン|턴|Runde|Tour|Turno|Turno|Ход|Tur",
L"修改时间|Modified|更新日時|수정 시간|Geändert|Modifié le|Modificado|Modificado|Изменено|Değiştirilme",
L"存档列表|Saves|セーブ一覧|저장 목록|Spielstände|Sauvegardes|Partidas|Jogos salvos|Сохранения|Kayıtlar",
L"存档搜索|Save search|セーブ検索|저장 검색|Spielstandsuche|Recherche de sauvegardes|Buscar partidas|Buscar jogos salvos|Поиск сохранений|Kayıt arama",
L"清除搜索|Clear search|検索を消去|검색 지우기|Suche leeren|Effacer la recherche|Borrar búsqueda|Limpar pesquisa|Очистить поиск|Aramayı temizle",
L"文件名、战役、回合或时间|File, campaign, turn or time|ファイル名・戦役・ターン・日時|파일명·캠페인·턴·시간|Datei, Kampagne, Runde oder Zeit|Fichier, campagne, tour ou heure|Archivo, campaña, turno u hora|Arquivo, campanha, turno ou hora|Файл, кампания, ход или время|Dosya, senaryo, tur veya saat",
L"自动存档|Autosave|オートセーブ|자동 저장|Automatischer Spielstand|Sauvegarde automatique|Guardado automático|Salvamento automático|Автосохранение|Otomatik kayıt",
L"手动存档|Manual save|手動セーブ|수동 저장|Manueller Spielstand|Sauvegarde manuelle|Guardado manual|Salvamento manual|Ручное сохранение|Manuel kayıt",
L"当前存档|Active save|現在のセーブ|현재 저장|Aktiver Spielstand|Sauvegarde active|Partida activa|Jogo salvo ativo|Активное сохранение|Etkin kayıt",
L"回合存档|Turn save|ターンセーブ|턴 저장|Rundenspielstand|Sauvegarde de tour|Guardado de turno|Salvamento de turno|Сохранение хода|Tur kaydı",
L"最终存档|Final save|最終セーブ|최종 저장|Abschlussspielstand|Sauvegarde finale|Guardado final|Salvamento final|Финальное сохранение|Son kayıt",
L"搜索文件名或战役|Search file or campaign|ファイル名・キャンペーン検索|파일 또는 캠페인 검색|Datei oder Kampagne suchen|Fichier ou campagne|Archivo o campaña|Arquivo ou campanha|Файл или кампания|Dosya veya senaryo ara",
L"选择存档文件夹|Choose save folder|セーブフォルダーを選択|저장 폴더 선택|Spielstandordner wählen|Choisir le dossier|Elegir carpeta de partidas|Escolher pasta de jogos|Выбрать папку сохранений|Kayıt klasörünü seç",
L"正在搜索…|Searching…|検索中…|검색 중…|Suche läuft…|Recherche…|Buscando…|Buscando…|Поиск…|Aranıyor…",
L"没有匹配的存档|No matching saves|一致するセーブなし|일치하는 저장 없음|Keine passenden Spielstände|Aucune sauvegarde trouvée|Sin partidas coincidentes|Nenhum jogo correspondente|Совпадений нет|Eşleşen kayıt yok",
L"未找到里奇亚存档，可添加文件夹|No Rizia saves. Add a folder.|Riziaセーブなし。フォルダーを追加。|Rizia 저장 없음. 폴더를 추가하세요.|Keine Rizia-Spielstände. Ordner hinzufügen.|Aucune sauvegarde Rizia. Ajoutez un dossier.|Sin partidas Rizia. Añada una carpeta.|Nenhum jogo Rizia. Adicione uma pasta.|Нет сохранений Rizia. Добавьте папку.|Rizia kaydı yok. Klasör ekleyin.",
L"未找到存档，可添加文件夹|No saves. Add a folder.|セーブなし。フォルダーを追加。|저장 없음. 폴더를 추가하세요.|Keine Spielstände. Ordner hinzufügen.|Aucune sauvegarde. Ajoutez un dossier.|Sin partidas. Añada una carpeta.|Nenhum jogo salvo. Adicione uma pasta.|Нет сохранений. Добавьте папку.|Kayıt yok. Klasör ekleyin.",
L"搜索失败，请刷新或添加文件夹|Search failed. Refresh or add a folder.|検索失敗。更新かフォルダー追加。|검색 실패. 새로 고침하거나 폴더를 추가하세요.|Suche fehlgeschlagen. Aktualisieren oder Ordner hinzufügen.|Échec. Actualisez ou ajoutez un dossier.|Error. Actualice o añada una carpeta.|Falha. Atualize ou adicione uma pasta.|Сбой поиска. Обновите или добавьте папку.|Arama başarısız. Yenileyin veya klasör ekleyin.",
L"经济|Economy|経済|경제|Wirtschaft|Économie|Economía|Economia|Экономика|Ekonomi",
L"政治|Politics|政治|정치|Politik|Politique|Política|Política|Политика|Siyaset",
L"人物|People|人物|인물|Personen|Personnages|Personajes|Personagens|Персонажи|Kişiler",
L"外交与军事|Diplomacy / army|外交・軍事|외교·군사|Diplomatie / Heer|Diplomatie / armée|Diplomacia / ejército|Diplomacia / exército|Дипломатия / армия|Diplomasi / ordu",
L"索德兰|Sordland|ソードランド|소르들란트|Sordland|Sordland|Sordland|Sordland|Сордланд|Sordland",
L"里奇亚|Rizia|リジア|리지아|Rizia|Rizia|Rizia|Rizia|Ризия|Rizia",
L"语言|Language|言語|언어|Sprache|Langue|Idioma|Idioma|Язык|Dil",
L"个人财富|Wealth|個人資産|개인 재산|Vermögen|Fortune|Patrimonio|Patrimônio|Состояние|Servet",
L"预算|Budget|予算|예산|Budget|Budget|Presupuesto|Orçamento|Бюджет|Bütçe",
L"经济水平|Economy level|経済水準|경제 수준|Wirtschaftsniveau|Niveau économique|Nivel económico|Nível econômico|Уровень экономики|Ekonomi düzeyi",
L"导出 CSV|Export CSV|CSV出力|CSV 내보내기|CSV exportieren|Exporter CSV|Exportar CSV|Exportar CSV|Экспорт CSV|CSV dışa aktar",
L"开|On|オン|켜짐|Ein|Activé|Activado|Ativado|Вкл.|Açık",
L"关|Off|オフ|꺼짐|Aus|Désactivé|Desactivado|Desativado|Выкл.|Kapalı",
L"资源与经济|Resources|資源・経済|자원·경제|Ressourcen|Ressources|Recursos|Recursos|Ресурсы|Kaynaklar",
L"军备库存|Military stock|軍備在庫|군수 재고|Militärbestände|Stock militaire|Equipo militar|Estoque militar|Военные запасы|Askerî stok",
L"家族与人物|Houses & people|貴族・人物|가문·인물|Häuser & Leute|Maisons / liens|Casas / vínculos|Casas / relações|Дома и связи|Hanedan / kişiler",
L"外交谈判|Diplomacy|外交|외교|Diplomatie|Diplomatie|Diplomacia|Diplomacia|Дипломатия|Diplomasi",
L"高级字段|Advanced|詳細変数|고급 필드|Erweitert|Avancé|Avanzado|Avançado|Расширенные|Gelişmiş",
L"修改预览|Review changes|変更確認|변경 검토|Prüfen|Vérifier|Revisar|Revisar|Изменения|İncele",
L"备份管理|Backups|バックアップ|백업|Sicherungen|Copies|Copias|Backups|Копии|Yedekler",
L"使用指南|Help|ヘルプ|도움말|Hilfe|Aide|Ayuda|Ajuda|Справка|Yardım",
L"组件许可|Licenses|ライセンス|라이선스|Lizenzen|Licences|Licencias|Licenças|Лицензии|Lisanslar",
L"战役|Campaign|キャンペーン|캠페인|Kampagne|Campagne|Campaña|Campanha|Кампания|Senaryo",
L"工作区|Workspace|ワークスペース|작업 공간|Arbeitsbereich|Espace de travail|Espacio de trabajo|Área de trabalho|Рабочая область|Çalışma alanı",
L"未打开存档|No save open|未読込|열린 저장 없음|Kein Spielstand geöffnet|Aucune sauvegarde ouverte|Ninguna partida abierta|Nenhum jogo aberto|Нет открытого сохранения|Açık kayıt yok",
L"未打开|Not opened|未読込|열리지 않음|Nicht geöffnet|Non ouvert|Sin abrir|Não aberto|Не открыто|Açılmadı",
L"选择一个项目|Select a field|項目を選択|항목 선택|Feld wählen|Choisir un champ|Elegir campo|Escolher campo|Выберите поле|Alan seç",
L"选择项目|Select field|項目選択|항목 선택|Feld wählen|Choisir un champ|Elegir campo|Escolher campo|Выберите поле|Alan seç",
L"项目|Field|項目|항목|Feld|Champ|Campo|Campo|Поле|Alan",
L"新值|New value|新しい値|새 값|Neuer Wert|Nouvelle valeur|Valor nuevo|Novo valor|Новое значение|Yeni değer",
L"原值|Original|元の値|원래 값|Original|Valeur initiale|Original|Original|Исходное|İlk değer",
L"待保存|Pending|未保存|저장 대기|Ausstehend|En attente|Pendiente|Pendente|Ожидает записи|Bekleyen",
L"类型|Type|種類|유형|Typ|Type|Tipo|Tipo|Тип|Tür",
L"数值|Number|数値|숫자|Zahl|Nombre|Número|Número|Число|Sayı",
L"开关|Boolean|真偽値|참/거짓|Boolesch|Booléen|Booleano|Booleano|Логическое|Mantıksal",
L"false / 否|false / No|false / いいえ|false / 아니요|false / Nein|false / Non|false / No|false / Não|false / Нет|false / Hayır",
L"true / 是|true / Yes|true / はい|true / 예|true / Ja|true / Oui|true / Sí|true / Sim|true / Да|true / Evet",
L"应用修改|Apply|適用|적용|Anwenden|Appliquer|Aplicar|Aplicar|Применить|Uygula",
L"还原该项|Reset field|項目を戻す|항목 복원|Zurücksetzen|Rétablir|Restablecer|Restaurar|Сбросить|Sıfırla",
L"复制字段名|Copy key|変数名コピー|필드명 복사|Kopieren|Copier la clé|Copiar clave|Copiar chave|Ключ|Anahtarı kopyala",
L"只看待保存|Pending only|未保存のみ|저장 대기만|Nur ausstehende|En attente|Solo pendientes|Só pendentes|Только изменения|Yalnız bekleyen",
L"只看收藏|Favorites|お気に入り|즐겨찾기|Favoriten|Favoris|Favoritos|Favoritos|Избранное|Favoriler",
L"收藏字段|Add favorite|お気に入りに追加|즐겨찾기 추가|Merken|Ajouter aux favoris|Añadir a favoritos|Favoritar|В избранное|Favoriye ekle",
L"取消收藏|Unfavorite|お気に入りを解除|즐겨찾기 해제|Entfernen|Retirer des favoris|Quitar de favoritos|Remover favorito|Из избранного|Favoriden çıkar",
L"无法保存字段收藏。|Could not save field favorites.|お気に入りを保存できませんでした。|즐겨찾기를 저장할 수 없습니다.|Favoriten konnten nicht gespeichert werden.|Impossible d’enregistrer les favoris.|No se pudieron guardar los favoritos.|Não foi possível salvar os favoritos.|Не удалось сохранить избранное.|Favoriler kaydedilemedi.",
L"当前存档没有已收藏的字段。关闭筛选后可选择字段收藏。|No favorites in this save. Turn off the filter and choose a field to add.|このセーブにお気に入りはありません。フィルターを解除して項目を追加してください。|이 저장 파일에는 즐겨찾기가 없습니다. 필터를 끄고 항목을 추가하세요.|Keine Favoriten in diesem Spielstand. Filter ausschalten und ein Feld hinzufügen.|Aucun favori dans cette sauvegarde. Désactivez le filtre pour ajouter un champ.|No hay favoritos en esta partida. Desactiva el filtro para añadir un campo.|Nenhum favorito neste save. Desative o filtro para adicionar um campo.|В этом сохранении нет избранных полей. Отключите фильтр и добавьте поле.|Bu kayıtta favori yok. Bir alan eklemek için filtreyi kapatın.",
L"导入方案|Import patch|設定を取込|설정 가져오기|Importieren|Importer|Importar|Importar|Импорт|İçe aktar",
L"导入修改方案|Import patch|設定を取込|설정 가져오기|Patch importieren|Importer les modifications|Importar ajustes|Importar ajustes|Импорт изменений|Yama içe aktar",
L"导出修改方案|Export patch|設定を出力|설정 내보내기|Exportieren|Exporter|Exportar|Exportar|Экспорт|Dışa aktar",
L"资源设为 99|Resources: 99|資源を99に|자원: 99|Ressourcen: 99|Ressources : 99|Recursos: 99|Recursos: 99|Ресурсы: 99|Kaynaklar: 99",
L"资源设为 999|Resources: 999|資源を999に|자원: 999|Ressourcen: 999|Ressources : 999|Recursos: 999|Recursos: 999|Ресурсы: 999|Kaynaklar: 999",
L"自动同步资源上限|Sync resource caps|資源上限を同期|자원 상한 동기화|Grenzen anpassen|Ajuster les plafonds|Ajustar límites|Ajustar limites|Синхр. лимиты|Kaynak sınırlarını eşle",
L"保存到原档|Save|保存|저장|Speichern|Enregistrer|Guardar|Salvar|Сохранить|Kaydet",
L"保存为|Save as|保存後|저장할 값|Speichern als|Valeur finale|Guardar como|Salvar como|Сохранить как|Farklı kaydet",
L"另存为…|Save as…|名前を付けて保存…|다른 이름으로 저장…|Speichern unter…|Enregistrer sous…|Guardar como…|Salvar como…|Сохранить как…|Farklı kaydet…",
L"清空待保存项|Clear changes|変更を消去|변경 지우기|Leeren|Effacer|Borrar cambios|Limpar alterações|Сбросить|Temizle",
L"打开备份目录|Backup folder|バックアップ先|백업 폴더|Sicherungsordner|Dossier de secours|Carpeta de copias|Pasta de backups|Папка копий|Yedek klasörü",
L"请在备份目录外另存一份存档。|Save a copy outside the backup folder.|バックアップフォルダーの外にコピーを保存してください。|백업 폴더 밖에 사본을 저장하세요.|Speichere eine Kopie außerhalb des Sicherungsordners.|Enregistrez une copie en dehors du dossier de sauvegarde.|Guarda una copia fuera de la carpeta de copias de seguridad.|Salve uma cópia fora da pasta de backups.|Сохраните копию вне папки резервных копий.|Bir kopyayı yedek klasörünün dışına kaydedin.",
L"恢复这份备份|Restore|復元|복원|Wiederherstellen|Restaurer|Restaurar|Restaurar|Восстановить|Geri yükle",
L"取消恢复|Cancel restore|復元取消|복원 취소|Abbrechen|Annuler|Cancelar|Cancelar|Отмена|İptal",
L"撤销|Undo|元に戻す|실행 취소|Rückgängig|Annuler|Deshacer|Desfazer|Отменить|Geri al",
L"重做|Redo|やり直す|다시 실행|Wiederholen|Rétablir|Rehacer|Refazer|Повторить|Yinele",
L"检查修改 →|Review →|変更確認 →|변경 검토 →|Prüfen →|Vérifier →|Revisar →|Revisar →|Проверить →|İncele →",
L"搜索项目或存档字段|Search fields|項目・変数を検索|필드 검색|Felder suchen|Rechercher un champ|Buscar campos|Buscar campos|Поиск полей|Alan ara",
L"撤回此项|Revert selected|選択項目を戻す|선택 변경 취소|Auswahl zurücksetzen|Annuler la modification|Revertir selección|Reverter seleção|Отменить выбранное|Seçili değişikliği geri al",
L"撤回关联库存|Revert linked stock|連動する在庫を戻す|연결된 재고 되돌리기|Bestand zurücksetzen|Rétablir le stock lié|Revertir existencias|Reverter estoque vinculado|Отменить изменение запаса|Bağlı stoku geri al",
L"此上限随库存自动调整。撤回关联库存可取消这两项修改。|This cap follows the stock value. Reverting the linked stock removes both changes.|この上限は在庫に連動します。在庫の変更を戻すと、両方の変更が取り消されます。|이 상한은 재고와 연동됩니다. 재고 변경을 되돌리면 두 변경 모두 취소됩니다.|Diese Obergrenze folgt dem Bestand. Beim Zurücksetzen des Bestands entfallen beide Änderungen.|Cette limite suit le stock. Rétablir le stock annule les deux modifications.|Este límite depende de las existencias. Revertirlas cancela ambos cambios.|Este limite acompanha o estoque. Reverter o estoque cancela as duas alterações.|Этот предел зависит от запаса. Отмена изменения запаса отменит оба изменения.|Bu sınır stok miktarına bağlıdır. Stok değişikliği geri alındığında iki değişiklik de kaldırılır.",
L"已撤回所选修改|Selected change reverted|選択した変更を取り消しました|선택한 변경을 취소했습니다|Ausgewählte Änderung zurückgesetzt|Modification sélectionnée annulée|Cambio seleccionado revertido|Alteração selecionada revertida|Выбранное изменение отменено|Seçili değişiklik geri alındı",
L"没有可调整的数值|No values to adjust|調整できる値はありません|조정할 값이 없습니다|Keine Werte anzupassen|Aucune valeur à ajuster|No hay valores que ajustar|Nenhum valor a ajustar|Нет значений для изменения|Ayarlanacak değer yok",
L"备份时间|Backup time|作成日時|백업 시간|Sicherungszeit|Date de copie|Fecha de copia|Data do backup|Время копии|Yedek zamanı",
L"大小|Size|サイズ|크기|Größe|Taille|Tamaño|Tamanho|Размер|Boyut",
L"原档备份|Backup file|バックアップ名|백업 파일|Sicherungsdatei|Fichier de secours|Archivo de copia|Arquivo de backup|Файл копии|Yedek dosyası",
L"备份详情|Backup details|バックアップ詳細|백업 정보|Sicherungsdetails|Détails de la copie|Detalles de copia|Detalhes do backup|Сведения о копии|Yedek ayrıntıları",
L"选择一份备份|Select a backup|バックアップ選択|백업 선택|Sicherung wählen|Choisir une copie|Elegir una copia|Escolher backup|Выберите копию|Yedek seç",
L"将修改的项目|Changed fields|変更項目|변경 항목|Geänderte Felder|Champs modifiés|Campos modificados|Campos alterados|Изменяемые поля|Değişen alanlar",
L"没有待保存项|No pending changes|変更なし|대기 중인 변경 없음|Keine Änderungen|Aucune modification|Sin cambios pendientes|Sem alterações pendentes|Нет изменений|Bekleyen değişiklik yok",
L"没有需要保存的修改|No changes to save|保存する変更なし|저장할 변경 없음|Nichts zu speichern|Rien à enregistrer|Sin cambios que guardar|Nada para salvar|Нечего сохранять|Kaydedilecek değişiklik yok",
L"暂无修改|No changes|変更なし|변경 없음|Keine Änderungen|Aucune modification|Sin cambios|Sem alterações|Нет изменений|Değişiklik yok",
L"暂无备份|No backups|バックアップなし|백업 없음|Keine Sicherungen|Aucune copie|Sin copias|Sem backups|Нет резервных копий|Yedek yok",
L"无匹配项目|No matching fields|一致する項目なし|일치하는 항목 없음|Keine passenden Felder|Aucun champ trouvé|Sin coincidencias|Sem correspondências|Совпадений нет|Eşleşen alan yok",
L"选择存档，或拖入 JSON 文件|Select a save or drop a JSON file|セーブを選択、またはJSONをドロップ|저장을 선택하거나 JSON 파일을 놓으세요|Spielstand wählen oder JSON ablegen|Choisissez une sauvegarde ou déposez un JSON|Elija una partida o arrastre un JSON|Selecione um jogo ou arraste um JSON|Выберите сохранение или перетащите JSON|Kayıt seçin veya JSON dosyasını bırakın",
L"未保存|Unsaved|未保存|저장되지 않음|Nicht gespeichert|Non enregistré|Sin guardar|Não salvo|Не сохранено|Kaydedilmedi",
L"已应用，待保存|Applied, not saved|適用済み・未保存|적용됨, 저장 필요|Angewendet, ungespeichert|Appliqué, non enregistré|Aplicado, sin guardar|Aplicado, não salvo|Применено, не сохранено|Uygulandı, kaydedilmedi",
L"资源已调整，待保存|Resources changed, not saved|資源変更済み・未保存|자원 변경됨, 저장 필요|Ressourcen geändert, ungespeichert|Ressources modifiées, non enregistrées|Recursos cambiados, sin guardar|Recursos alterados, não salvos|Ресурсы изменены, не сохранены|Kaynaklar değişti, kaydedilmedi",
L"方案已导入，待保存|Patch imported, not saved|設定取込済み・未保存|설정 가져옴, 저장 필요|Patch importiert, ungespeichert|Importé, non enregistré|Ajustes importados, sin guardar|Ajustes importados, não salvos|Изменения импортированы|Yama alındı, kaydedilmedi",
L"已撤销上一步修改|Change undone|元に戻しました|변경 취소됨|Rückgängig gemacht|Modification annulée|Cambio deshecho|Alteração desfeita|Изменение отменено|Değişiklik geri alındı",
L"已重做上一步修改|Change redone|やり直しました|변경 다시 실행됨|Wiederholt|Modification rétablie|Cambio rehecho|Alteração refeita|Изменение повторено|Değişiklik yinelendi",
L"字段名已复制|Key copied|変数名をコピー|필드명 복사됨|Schlüssel kopiert|Clé copiée|Clave copiada|Chave copiada|Ключ скопирован|Anahtar kopyalandı",
L"已取消恢复|Restore cancelled|復元を取消|복원 취소됨|Wiederherstellung abgebrochen|Restauration annulée|Restauración cancelada|Restauração cancelada|Восстановление отменено|Geri yükleme iptal edildi",
L"权威|Authority|権威|권위|Autorität|Autorité|Autoridad|Autoridade|Авторитет|Otorite",
L"能源|Energy|エネルギー|에너지|Energie|Énergie|Energía|Energia|Энергия|Enerji",
L"预算上限|Budget cap|予算上限|예산 상한|Budgetgrenze|Plafond budgétaire|Límite presupuestario|Limite orçamentário|Лимит бюджета|Bütçe sınırı",
L"权威上限|Authority cap|権威上限|권위 상한|Autoritätsgrenze|Plafond d'autorité|Límite de autoridad|Limite de autoridade|Лимит авторитета|Otorite sınırı",
L"能源上限|Energy cap|エネルギー上限|에너지 상한|Energiegrenze|Plafond d'énergie|Límite de energía|Limite de energia|Лимит энергии|Enerji sınırı",
L"兵员|Manpower|兵員|병력|Personal|Effectifs|Efectivos|Efetivos|Личный состав|İnsan gücü",
L"军事装备|Equipment|軍事装備|군사 장비|Ausrüstung|Équipement|Equipo|Equipamento|Снаряжение|Teçhizat",
L"卡车 / 支援装备|Trucks|トラック|트럭|Lastwagen|Camions|Camiones|Caminhões|Грузовики|Kamyonlar",
L"坦克库存|Tanks|戦車|전차|Panzer|Chars|Tanques|Tanques|Танки|Tanklar",
L"舰艇库存|Ships|艦艇|함정|Schiffe|Navires|Buques|Navios|Корабли|Gemiler",
L"潜艇库存|Submarines|潜水艦|잠수함|U-Boote|Sous-marins|Submarinos|Submarinos|Подлодки|Denizaltılar",
L"战斗机库存|Fighters|戦闘機|전투기|Jagdflugzeuge|Chasseurs|Cazas|Caças|Истребители|Avcı uçakları",
L"轰炸机库存|Bombers|爆撃機|폭격기|Bomber|Bombardiers|Bombarderos|Bombardeiros|Бомбардировщики|Bombardıman uçakları",
L"公众支持度|Public support|国民支持|국민 지지도|Öffentliche Zustimmung|Soutien populaire|Apoyo público|Apoio público|Поддержка народа|Halk desteği",
L"托拉斯家族支持度|House Toras|トラス家|토라스 가문|Haus Toras|Maison Toras|Casa Toras|Casa Toras|Дом Торас|Toras Hanedanı",
L"阿扎罗家族支持度|House Azaro|アザロ家|아자로 가문|Haus Azaro|Maison Azaro|Casa Azaro|Casa Azaro|Дом Азаро|Azaro Hanedanı",
L"萨宗家族支持度|House Sazon|サゾン家|사존 가문|Haus Sazon|Maison Sazon|Casa Sazon|Casa Sazon|Дом Сазон|Sazon Hanedanı",
L"未知时间|Unknown time|日時不明|시간 알 수 없음|Unbekannte Zeit|Date inconnue|Fecha desconocida|Data desconhecida|Время неизвестно|Bilinmeyen zaman",
L"还有未保存的修改。放弃这些修改并继续吗？|Discard unsaved changes and continue?|未保存の変更を破棄して続行しますか？|저장하지 않은 변경을 버리고 계속할까요?|Ungespeicherte Änderungen verwerfen?|Abandonner les modifications non enregistrées ?|¿Descartar cambios sin guardar?|Descartar alterações não salvas?|Отменить несохранённые изменения?|Kaydedilmeyen değişiklikler silinsin mi?",
L"确认恢复并备份当前档|Confirm restore|復元を確定|복원 확인|Wiederherstellen|Confirmer|Confirmar restauración|Confirmar restauração|Подтвердить|Geri yüklemeyi onayla",
L"请先退出 Suzerain，再保存或恢复存档。|Close Suzerain before saving or restoring.|保存・復元の前にSuzerainを終了してください。|저장 또는 복원 전에 Suzerain을 종료하세요.|Suzerain vor dem Speichern oder Wiederherstellen schließen.|Fermez Suzerain avant d'enregistrer ou restaurer.|Cierre Suzerain antes de guardar o restaurar.|Feche Suzerain antes de salvar ou restaurar.|Закройте Suzerain перед записью или восстановлением.|Kaydetmeden veya geri yüklemeden önce Suzerain'i kapatın.",
L"文件读取不完整，请重新打开。|Incomplete file read. Open it again.|読込が不完全です。開き直してください。|파일 읽기가 불완전합니다. 다시 여세요.|Datei unvollständig gelesen. Erneut öffnen.|Lecture incomplète. Rouvrez le fichier.|Lectura incompleta. Vuelva a abrir.|Leitura incompleta. Abra novamente.|Неполное чтение файла. Откройте снова.|Dosya tam okunamadı. Yeniden açın.",
L"请选择非空且不超过 50 MB 的游戏存档。|Select a nonempty save under 50 MB.|空でない50MB以下のセーブを選択。|비어 있지 않은 50MB 이하 저장을 선택하세요.|Nichtleeren Spielstand unter 50 MB wählen.|Choisissez un fichier non vide de moins de 50 Mo.|Elija una partida no vacía de menos de 50 MB.|Escolha um jogo não vazio de até 50 MB.|Выберите непустой файл до 50 МБ.|50 MB altı, boş olmayan kayıt seçin.",
L"备份校验失败，本次没有覆盖存档。|Backup verification failed. Save unchanged.|バックアップ検証失敗。元のセーブは未変更。|백업 검증 실패. 저장은 변경되지 않았습니다.|Sicherungsprüfung fehlgeschlagen. Spielstand unverändert.|Échec de vérification. Sauvegarde inchangée.|Error al verificar copia. Partida intacta.|Falha na verificação. Jogo inalterado.|Проверка копии не удалась. Файл не изменён.|Yedek doğrulanamadı. Kayıt değişmedi.",
L"保存前自动备份。|A backup is created before saving.|保存前に自動バックアップ。|저장 전에 자동 백업됩니다.|Vor dem Speichern wird gesichert.|Une copie est créée avant l'enregistrement.|Se crea una copia antes de guardar.|Um backup é criado antes de salvar.|Перед записью создаётся копия.|Kaydetmeden önce yedek oluşturulur.",
L"使用|Usage|使い方|사용법|Bedienung|Utilisation|Uso|Uso|Использование|Kullanım",
L"快捷键|Shortcuts|ショートカット|단축키|Tastenkürzel|Raccourcis|Atajos|Atalhos|Горячие клавиши|Kısayollar",
L"另一个窗口正在保存此存档，请稍后重试。|Another window is writing this save. Try again shortly.|別のウィンドウが保存中です。しばらくしてからもう一度お試しください。|다른 창에서 저장 중입니다. 잠시 후 다시 시도하세요.|Ein anderes Fenster speichert gerade. Bitte gleich erneut versuchen.|Une autre fenêtre écrit cette sauvegarde. Réessayez dans un instant.|Otra ventana está guardando. Vuelve a intentarlo en un momento.|Outra janela está salvando. Tente de novo em instantes.|Другое окно записывает это сохранение. Попробуйте чуть позже.|Başka bir pencere bu kaydı yazıyor. Biraz sonra tekrar deneyin.",
L"未完成游戏读档验证。|In-game loading has not been verified.|ゲーム内読込は未検証です。|게임 내 불러오기는 검증되지 않았습니다.|Laden im Spiel noch nicht geprüft.|Chargement en jeu non vérifié.|Carga dentro del juego no verificada.|Carregamento no jogo não verificado.|Загрузка в игре не проверена.|Oyun içinde yükleme doğrulanmadı.",
L"预算 +5|Budget +5|予算 +5|예산 +5|Budget +5|Budget +5|Presupuesto +5|Orçamento +5|Бюджет +5|Bütçe +5",
L"财富 +5|Wealth +5|資産 +5|재산 +5|Vermögen +5|Fortune +5|Patrimonio +5|Patrimônio +5|Состояние +5|Servet +5",
L"不支持的战役。|Unsupported campaign.|未対応のキャンペーン。|지원되지 않는 캠페인입니다.|Nicht unterstützte Kampagne.|Campagne non prise en charge.|Campaña no compatible.|Campanha não compatível.|Кампания не поддерживается.|Desteklenmeyen senaryo.",
L"战役不匹配。|Campaign mismatch.|キャンペーンが一致しません。|캠페인이 일치하지 않습니다.|Kampagne stimmt nicht überein.|Campagne différente.|La campaña no coincide.|Campanha diferente.|Кампания не совпадает.|Senaryo eşleşmiyor.",
L"未找到存档，请添加文件夹。|No saves found. Add a folder.|セーブなし。フォルダーを追加。|저장 없음. 폴더를 추가하세요.|Keine Spielstände. Ordner hinzufügen.|Aucune sauvegarde. Ajoutez un dossier.|Sin partidas. Añada una carpeta.|Nenhum jogo salvo. Adicione uma pasta.|Нет сохранений. Добавьте папку.|Kayıt yok. Klasör ekleyin.",
L"CSV 已导出。|CSV exported.|CSVを出力しました。|CSV를 내보냈습니다.|CSV exportiert.|CSV exporté.|CSV exportado.|CSV exportado.|CSV экспортирован.|CSV dışa aktarıldı.",
L"请在备份目录外选择 .csv 文件。|Choose a .csv file outside the backup folder.|バックアップ先以外の.csvファイルを選択。|백업 폴더 밖의 .csv 파일을 선택하세요.|CSV-Datei außerhalb des Sicherungsordners wählen.|Choisissez un .csv hors du dossier de secours.|Elija un .csv fuera de la carpeta de copias.|Escolha um .csv fora da pasta de backups.|Выберите .csv вне папки резервных копий.|Yedek klasörü dışında bir .csv dosyası seçin.",
L"拖入 Suzerain JSON 存档。|Drop a Suzerain JSON save here.|SuzerainのJSONセーブをドロップ。|Suzerain JSON 저장 파일을 놓으세요.|Suzerain-JSON-Spielstand hier ablegen.|Déposez une sauvegarde JSON Suzerain.|Arrastre una partida JSON de Suzerain.|Arraste um jogo JSON de Suzerain.|Перетащите JSON-сохранение Suzerain.|Suzerain JSON kaydını buraya bırakın.",
L"已打开  ·  |Opened · |読込済み · |열림 · |Geöffnet · |Ouvert · |Abierto · |Aberto · |Открыто · |Açıldı · ",
L"原值   |Original   |元の値   |원래 값   |Original   |Valeur initiale   |Original   |Original   |Исходное   |İlk değer   ",
L"上限 |Cap |上限 |상한 |Grenze |Plafond |Límite |Limite |Лимит |Sınır ",
L"净变化 |Net |純増減 |순변화 |Netto |Net |Neto |Líquido |Изменение |Net ",
L"当前预算|Budget balance|現在の予算|현재 예산|Budgetstand|Budget actuel|Saldo actual|Saldo atual|Бюджет|Mevcut bütçe",
L"当前权威|Authority on hand|現在の権威|현재 권위|Autorität|Autorité actuelle|Autoridad actual|Autoridade atual|Авторитет|Mevcut otorite",
L"当前能源|Energy on hand|現在のエネルギー|현재 에너지|Energievorrat|Énergie disponible|Energía disponible|Energia disponível|Запас энергии|Mevcut enerji",
L"每回合|Per turn|ターン収支|턴당|Pro Runde|Par tour|Por turno|Por turno|За ход|Tur başına",
L"这份存档未列出该资源的收支明细。|This save contains no individual modifiers for this resource.|このセーブには、この資源の増減の内訳がありません。|이 저장 파일에는 해당 자원의 증감 항목이 없습니다.|Dieser Spielstand enthält keine einzelnen Modifikatoren für diese Ressource.|Cette sauvegarde ne contient aucun modificateur individuel pour cette ressource.|Esta partida no contiene modificadores individuales para este recurso.|Este jogo salvo não contém modificadores individuais para este recurso.|В сохранении нет отдельных модификаторов этого ресурса.|Bu kayıtta bu kaynağı etkileyen ayrı kalemler bulunmuyor.",
L"这是游戏计算的合计值，事件发生时可能重算。修改它不能锁定每回合收入。|The game calculates this total and may update it after events. A change here can be temporary.|ゲームが計算した合計値です。イベント後の再計算で変更が上書きされる場合があります。|게임이 계산한 합계입니다. 이벤트 후 다시 계산하면 수정한 값이 바뀔 수 있습니다.|Das Spiel berechnet diese Summe. Ereignisse können den geänderten Wert wieder überschreiben.|Le jeu calcule ce total. Les événements peuvent ensuite remplacer la valeur modifiée.|El juego calcula este total. Los eventos pueden volver a cambiar el valor editado.|O jogo calcula esse total. Os eventos podem substituir o valor editado.|Игра вычисляет эту сумму. После событий изменённое значение может быть пересчитано.|Bu toplamı oyun hesaplar. Olaylardan sonra düzenlenen değer yeniden hesaplanabilir.",
L"在高级字段中搜索以下前缀，可查看收支明细。|Search Advanced fields with this prefix to see individual modifiers.|詳細変数で次の接頭辞を検索すると、増減の内訳を確認できます。|고급 필드에서 아래 접두어를 검색하면 증감 항목을 볼 수 있습니다.|Unter Erweitert nach diesem Präfix suchen, um die einzelnen Modifikatoren zu sehen.|Recherchez ce préfixe dans les champs avancés pour voir le détail des modificateurs.|Busca este prefijo en los campos avanzados para ver los modificadores.|Pesquise este prefixo nos campos avançados para ver os modificadores.|Найдите этот префикс в расширенных полях, чтобы увидеть отдельные модификаторы.|Etkenleri görmek için gelişmiş alanlarda bu öneki arayın.",
L"预览版  |Preview  |プレビュー  |미리 보기  |Vorschau  |Aperçu  |Vista previa  |Prévia  |Предварительная  |Önizleme  ",
L"目标：|Target:|保存先：|대상:|Ziel:|Cible :|Destino:|Destino:|Назначение:|Hedef:",
L" 个存档 · 最新在前| saves · newest first| 件 · 新しい順|개 저장 · 최신순| Spielstände · neueste zuerst| sauvegardes · récentes d'abord| partidas · recientes primero| jogos · recentes primeiro| сохр. · новые первыми| kayıt · yeniden eskiye",
L"1 项即将保存|1 change to save|1 件の変更|저장할 변경 1개|1 Änderung zu speichern|1 modification à enregistrer|1 cambio para guardar|1 alteração para salvar|1 изменение для сохранения|Kaydedilecek 1 değişiklik",
L" 项即将保存| changes to save| 件の変更|개 변경 저장 예정| Änderungen zu speichern| modifications à enregistrer| cambios por guardar| alterações para salvar| изменений к записи| değişiklik kaydedilecek",
L" · 已到搜索上限| · search limit reached| · 検索上限に到達| · 검색 한도 도달| · Suchlimit erreicht| · limite atteinte| · límite alcanzado| · limite atingido| · достигнут лимит| · arama sınırına ulaşıldı",
L" · 部分文件无法读取| · some files unreadable| · 一部読込不可| · 일부 파일 읽기 실패| · einige Dateien nicht lesbar| · certains fichiers illisibles| · algunos archivos ilegibles| · alguns arquivos ilegíveis| · часть файлов недоступна| · bazı dosyalar okunamadı",
L"回合  |Turn  |ターン  |턴  |Runde  |Tour  |Turno  |Turno  |Ход  |Tur  "
        };
        for(auto row:data){Entry e;std::wstring value=row;size_t at=0;while(true){auto next=value.find(L'|',at);e.text.push_back(value.substr(at,next==std::wstring::npos?next:next-at));if(next==std::wstring::npos)break;at=next+1;}if(e.text.size()<2)continue;out.emplace(e.text[0],e);out.emplace(e.text[1],e);}
        out.emplace(L"Diplomacy & military",out.at(L"外交与军事"));
        out.emplace(L"王国管理",out.at(L"战役"));
        return out;
    }();return rows;
}
inline std::wstring entryText(const Entry& e){if(language==0)return e.text[0];if(language==1)return traditional(e.text[0]);size_t index=(size_t)language-1;return index<e.text.size()&&!e.text[index].empty()?e.text[index]:e.text[1];}
// Longer explanations and rare error text deliberately fall back to English when untranslated.
inline const std::map<std::wstring,std::wstring>& englishFallback(){
    static const std::map<std::wstring,std::wstring> rows={
        {L"里奇亚存档工坊",L"Rizia Save Studio"},
        {L"（无法显示的文本）",L"(Text unavailable)"},
        {L"Windows 错误",L"Windows error"},
        {L"只允许写入里奇亚 DLC 存档。",L"Only Rizia DLC saves can be written."},
        {L"原存档在打开后发生了变化。为避免覆盖游戏的新进度，请重新打开该存档再修改。",L"The save changed after opening. Reload it before editing."},
        {L"写入校验失败，原档仍保留。",L"Write verification failed. The original save is unchanged."},
        {L"保存过程中原存档又被其他程序修改，请退出游戏并重新打开。",L"The save changed while saving. Close the game and reopen the file."},
        {L"无法启动存档搜索。",L"Could not start save search."},
        {L"无法打开文件夹选择窗口。",L"Could not open the folder chooser."},
        {L"无法选择这个文件夹。",L"Could not select this folder."},
        {L"搜索未完成，请重试。",L"Search did not finish. Please retry."},
        {L"不是里奇亚备份。",L"This is not a Rizia backup."},
        {L"不是里奇亚存档。",L"This is not a Rizia save."},
        {L"无法读取这份备份",L"Cannot read this backup"},
        {L"注意：战役名称与当前存档不同。",L"Warning: the campaign name differs from the current save."},
        {L"目标文件不是里奇亚存档，请选择其他名称。",L"The destination is not a Rizia save. Choose another name."},
        {L"这个文件不是里奇亚 DLC 存档。请选择里奇亚战役的 JSON 文件。",L"This is not a Rizia DLC save. Choose a Rizia campaign JSON file."},
        {L"已另存。游戏未显示自定义文件名时，请使用原存档名称和位置。",L"Saved as a new file. If it is missing in-game, use the original save name and location."},
        {L"已保存并校验备份。请回到游戏主菜单，手动读取这份存档。",L"Saved and backup verified. Load this save from the game’s main menu."},
        {L"再次确认后，将用选中备份替换当前文件；当前版本仍会另做备份。",L"Confirm to replace the current save with this backup. The current version will also be backed up."},
        {L"这份备份在预览后发生了变化。请刷新列表，重新确认后再恢复。",L"The backup changed after preview. Refresh and confirm it again."},
        {L"备份已恢复。恢复前的版本也已保存，可在列表中回退。",L"Backup restored. The previous version was also backed up."},
        {L"方案写入校验失败。",L"Patch write verification failed."},
        {L"没有可导出的修改，请先编辑项目。",L"No changes to export. Edit a field first."},
        {L"请用 .txt 文件导出方案，不能覆盖游戏存档或备份目录。",L"Export patches as .txt files, outside save and backup files."},
        {L"修改方案已导出，只包含字段和值。",L"Patch exported with field names and values."},
        {L"待保存项已清空，点击“撤销”可恢复",L"Changes cleared. Undo to restore them."},
        {L"首次保存后将创建备份目录",L"The backup folder is created on first save."},
        {L"请拖入里奇亚 JSON 存档。修改方案可在高级字段页导入。",L"Drop a Rizia JSON save. Import patches from Advanced."},
        {L"操作没有完成。请重新打开存档后重试。",L"Operation failed. Reopen the save and try again."},
        {L"资源、上限和军需请输入 0 或正整数，不使用小数、符号或科学计数法。",L"Resources, caps and military stock require nonnegative whole numbers."},
        {L"只修改此字段，不联动其他条件。",L"Only this field is changed. Related conditions remain unchanged."},
        {L"仅修改此字段，剧情条件不联动。",L"Only this field is changed. Story conditions remain unchanged."},
        {L"这是游戏计算的合计值，事件发生时可能重算。修改它不能锁定每回合收入。",L"The game may recalculate this total. Editing it does not lock income per turn."},
        {L"部队状态与部署、阵亡、训练和战棋数据相互关联。军备库存页更适合常规修改。",L"Unit state depends on deployment, casualties, training and battle data. Use Military stock for normal edits."},
        {L"选择存档 → 修改数值 → 修改预览 → 保存。",L"Select a save → Edit values → Review changes → Save."},
        {L"保存前退出游戏，保存后在游戏中重新读档。",L"Close the game before saving, then reload your save in-game."},
        {L"备份可在“备份管理”中恢复。",L"Restore saved backups from Backups."},
        {L"Ctrl+L  选择存档     Ctrl+O  手动打开",L"Ctrl+L  Select save     Ctrl+O  Open file"},
        {L"Ctrl+S  修改预览     Ctrl+F  搜索字段",L"Ctrl+S  Review changes     Ctrl+F  Search fields"},
        {L"Ctrl+R  重新载入     Ctrl+Z / Y  撤销 / 重做",L"Ctrl+R  Reload     Ctrl+Z / Y  Undo / Redo"},
        {L"Enter  应用输入",L"Enter  Apply value"},
        {L"当前可花费的预算库存。",L"Current spendable budget."},
        {L"当前可用于法令与决策的权威库存。",L"Authority available for decrees and decisions."},
        {L"当前能源库存。每回合的生产和消耗仍由游戏计算。",L"Current energy stock. Production and consumption remain game-controlled."},
        {L"当前存档的预算上限；后续游戏事件仍可能重算。",L"Budget cap. Later game events may recalculate it."},
        {L"当前存档的权威上限；后续游戏事件仍可能重算。",L"Authority cap. Later game events may recalculate it."},
        {L"当前存档的能源上限；后续游戏事件仍可能重算。",L"Energy cap. Later game events may recalculate it."},
        {L"可用兵员库存。不是已部署的师团数量。",L"Available manpower, not deployed divisions."},
        {L"军需库存。修改后仍需按游戏规则训练部队。",L"Military stock. Units still require training in-game."},
        {L"军需库存，不会自动生成支援师。",L"Military stock. Does not create support divisions."},
        {L"用于训练的坦克库存，不直接改战棋中的部队。",L"Tank stock for training, not deployed units."},
        {L"用于训练的舰艇库存，不直接改战棋中的部队。",L"Ship stock for training, not deployed units."},
        {L"用于训练的潜艇库存，不直接改战棋中的部队。",L"Submarine stock for training, not deployed units."},
        {L"战斗机资源库存；战场状态保持不变。",L"Fighter stock. Battlefield state remains unchanged."},
        {L"轰炸机资源库存；战场状态保持不变。",L"Bomber stock. Battlefield state remains unchanged."},
        {L"数值不是百分比。既有政变、承诺和结盟事件仍由游戏判定。",L"Not a percentage. Coups, promises and alliances remain game-controlled."},
        {L"只改家族支持数值，不自动改婚约、军队或政变标记。",L"Changes house support only, not marriage, army or coup flags."},
        {L"只改家族支持数值，不自动改改革和复权事件。",L"Changes house support only, not reform or restoration events."},
        {L"民意数值不是百分比；生活水平和事件另算。",L"Public support is not a percentage. Living standards and events are separate."},
        {L"只改对应宗教群体的支持数值。",L"Changes support of this religious group only."},
        {L"只改关系数值，不自动改变婚姻或继承安排。",L"Changes the relationship value, not marriage or succession."},
        {L"只改关系数值，剧情条件另算。",L"Changes the relationship value. Story conditions are separate."},
        {L"只改关系数值，不自动开启恋爱标记。",L"Changes the relationship value, not romance flags."},
        {L"只改关系数值，不改变调查结果。",L"Changes the relationship value, not investigation results."},
        {L"只改关系数值，不保证消除既有政变条件。",L"Changes the relationship value without removing existing coup conditions."},
        {L"这是恋爱数值，不是通用好感；恋爱、婚姻和生育事件标记保持不变。",L"Romance value, not general approval. Event flags remain unchanged."},
        {L"只改谈判力数值，气田、婚盟和统一条件仍由游戏判定。",L"Changes negotiating power only. Gas-field, marriage and unification conditions remain game-controlled."},
        {L"只改谈判力数值，齐勒交还和贸易事件另算。",L"Changes negotiating power only. Zille and trade events are separate."},
        {L"只改谈判力数值，已作出的协议承诺保持不变。",L"Changes negotiating power only. Existing agreements remain unchanged."},
        {L"只改谈判力数值，宗教与外交事件另算。",L"Changes negotiating power only. Religious and diplomatic events are separate."},
        {L"只改谈判力数值，不改变本体存档和世界背景。",L"Changes negotiating power only, not base-game saves or world background."},
        {L"只改谈判力数值，GRACE 和亲属关系事件另算。",L"Changes negotiating power only. GRACE and family events are separate."},
        {L"只改谈判力数值，不自动更改军援或 MITZ 协议。",L"Changes negotiating power only, not military aid or MITZ agreements."},
        {L"只改谈判力数值，不自动更改证据、调查或制裁。",L"Changes negotiating power only, not evidence, investigations or sanctions."},
        {L"Wruhecist 宗教支持度",L"Wruhecist support"},{L"Golcondist 宗教支持度",L"Golcondist support"},{L"Dastnurist 宗教支持度",L"Dastnurist support"},
        {L"维娜关系（Vina）",L"Vina"},{L"雨果关系（Hugo）",L"Hugo"},{L"帕贝尔关系（Pabel）",L"Pabel"},{L"王太后关系（Estela）",L"Estela"},{L"萨尔关系（Sal）",L"Sal"},{L"提图斯关系（Titus）",L"Titus"},{L"露西塔恋爱数值",L"Lucita romance"},
        {L"帕莱斯谈判力",L"Pales negotiating power"},{L"韦伦谈判力",L"Wehlen negotiating power"},{L"莫雷拉谈判力",L"Morella negotiating power"},{L"德尔迪亚谈判力",L"Derdia negotiating power"},{L"索德兰谈判力",L"Sordland negotiating power"},{L"伦堡谈判力",L"Rumburg negotiating power"},{L"瓦尔格斯兰谈判力",L"Valgsland negotiating power"},{L"莱斯皮亚谈判力",L"Lespia negotiating power"},
        {L"另存修改后的存档",L"Save modified game as"},{L"打开里奇亚存档",L"Open Rizia save"},{L"所有文件",L"All files"}
    };return rows;
}
inline bool isPath(const std::wstring&s){return (s.size()>2&&((s[1]==L':'&&(s[2]==L'\\'||s[2]==L'/'))||(s[0]==L'\\'&&s[1]==L'\\')))||s.rfind(L"RiziaDLC.",0)==0||s.rfind(L"BaseGame.",0)==0;}
inline std::wstring translate(const std::wstring& source){
    if(source.empty()||isPath(source))return source;
    auto found=dictionary().find(source);if(found!=dictionary().end())return entryText(found->second);
    if(language<=1){std::wstring field=fieldZh(source.c_str());if(field!=source)return language==1?traditional(field):field;}
    if(language>=3){auto field=fieldWest(source,language);if(field!=source)return field;field=fieldEast(source,language);if(field!=source)return field;}
    auto fallback=englishFallback().find(source);if(fallback!=englishFallback().end())return language==0?source:language==1?traditional(source):fallback->second;
    auto end=source.find(L'\n');if(end!=std::wstring::npos){std::wstring out;size_t at=0;while(at<source.size()){auto n=source.find(L'\n',at);auto line=source.substr(at,n==std::wstring::npos?n:n-at);bool cr=!line.empty()&&line.back()==L'\r';if(cr)line.pop_back();out+=translate(line);if(cr)out+=L'\r';if(n==std::wstring::npos)break;out+=L'\n';at=n+1;}return out;}
    // A translated caption may append a numeric count (for example, Review changes 3).
    for(const auto& item:dictionary()){
        const auto& key=item.first;
        if(source.size()>key.size()&&source.compare(0,key.size(),key)==0){
            auto tail=source.substr(key.size());
            if((tail.front()==L' '||tail.front()==L'\t')&&tail.find_first_not_of(L" \t0123456789") == std::wstring::npos)
                return entryText(item.second)+tail;
        }
    }
    // Never rewrite arbitrary substrings: only recognized UI prefixes or suffixes may wrap data.
    const std::pair<const wchar_t*,const wchar_t*> prefixes[]={
        {L"已打开  ·  ",L"Opened · "},{L"原值   ",L"Original   "},{L"上限 ",L"Cap "},{L"净变化 ",L"Net "},{L"预览版  ",L"Preview  "},
        {L"目标：",L"Target:"},{L"无法读取文件：",L"Cannot read file: "},{L"无法创建原档备份，本次没有覆盖存档：",L"Cannot create backup. Save unchanged: "},
        {L"无法创建临时文件：",L"Cannot create temporary file: "},{L"写入失败：",L"Write failed: "},{L"无法完成写入：",L"Cannot finish writing: "},
        {L"Windows 无法替换存档，原档和备份仍保留：",L"Cannot replace save. Original and backup retained: "},
        {L"无法创建存档列表：",L"Cannot create save list: "},{L"无法打开存档选择窗口：",L"Cannot open save picker: "},
        {L"请返回编辑页调整：",L"Return to editing: "},{L"方案写入失败：",L"Patch write failed: "},
        {L"无法打开这个存档。",L"Cannot open this save."},{L"保存没有完成。",L"Save failed."},{L"恢复没有完成。",L"Restore failed."},
        {L"修改方案没有处理完成。",L"Patch operation failed."},{L"操作没有完成。",L"Operation failed."},{L"这个值还没有应用。",L"Value was not applied."},
        {L"默认目录不存在，请手动打开存档。",L"Default folder not found. Open a save manually."},
        {L"将恢复到当前打开的存档。",L"Restore into the current save."},{L"恢复前会再次备份当前文件。",L"The current file will be backed up first."},{L"回合  ",L"Turn  "}
    };
    for(const auto& p:prefixes){std::wstring key=p.first;if(source.rfind(key,0)==0){auto rest=source.substr(key.size());if(language==0)return source;if(language==1)return traditional(key)+rest;auto i=dictionary().find(key);return (i==dictionary().end()?std::wstring(p.second):entryText(i->second))+rest;}}
    const std::pair<const wchar_t*,const wchar_t*> suffixes[]={
        {L" 个存档 · 最新在前",L" saves · newest first"},{L" 项即将保存",L" changes to save"},
        {L" · 已到搜索上限",L" · search limit reached"},{L" · 部分文件无法读取",L" · some files unreadable"},
        {L"必须是非负十进制整数。",L" must be a nonnegative integer."},
        {L"超过上限。请开启自动同步上限，或手动修改上限。",L" exceeds the cap. Enable cap sync or edit the cap."},
        {L"不能低于当前或待保存的库存。请调整库存或上限。",L" cannot be lower than stock. Adjust stock or cap."}
    };
    for(const auto& p:suffixes){std::wstring key=p.first;if(source.size()>=key.size()&&source.compare(source.size()-key.size(),key.size(),key)==0){auto head=source.substr(0,source.size()-key.size());if(language==0)return source;if(language==1)return translate(head)+traditional(key);auto i=dictionary().find(key);return translate(head)+(i==dictionary().end()?std::wstring(p.second):entryText(i->second));}}
    // Non-UI data (campaign names, unknown game fields, paths) stays byte-for-byte unchanged.
    return source;
}
} // namespace i18n
inline std::wstring tr(const std::wstring& source){return i18n::translate(source);}
