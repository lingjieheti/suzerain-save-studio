#pragma once
#include "i18n.hpp"

namespace sponsor_text {
enum class Key { Title, Intro, WeChat, PayPal, USDT, ScanWeChat,
                 ScanPayPal, ScanUSDT, OpenPage, CopyAddress, Copied, Close,
                 CopyFailed, OpenFailed, QrUnavailable };

inline const wchar_t* get(Key key) {
    // The columns follow the desktop language selector in i18n::names.
    static constexpr const wchar_t* labels[][11]={
        {L"赞助作者",L"贊助作者",L"Support the author",L"作者を応援",L"개발자 후원",L"Autor unterstützen",L"Soutenir l’auteur",L"Apoyar al autor",L"Apoiar o autor",L"Поддержать автора",L"Geliştiriciye destek"},
        {L"如果这个工具帮到了你，欢迎请我喝杯咖啡。",L"如果這個工具幫上了忙，歡迎請我喝杯咖啡。",L"If this tool helped you, you can buy me a coffee.",L"このツールが役に立ったら、コーヒー一杯分の応援をいただけるとうれしいです。",L"이 도구가 도움이 되었다면 커피 한 잔으로 응원해 주세요.",L"Wenn dir das Tool geholfen hat, kannst du mir einen Kaffee spendieren.",L"Si cet outil vous a rendu service, vous pouvez m’offrir un café.",L"Si esta herramienta te ha ayudado, puedes invitarme a un café.",L"Se esta ferramenta foi útil, você pode me pagar um café.",L"Если этот редактор вам пригодился, можете угостить меня кофе.",L"Bu araç işinize yaradıysa bana bir kahve ısmarlayabilirsiniz."},
        {L"微信赞赏码",L"微信讚賞碼",L"WeChat",L"WeChat",L"WeChat",L"WeChat",L"WeChat",L"WeChat",L"WeChat",L"WeChat",L"WeChat"},
        {L"PayPal",L"PayPal",L"PayPal",L"PayPal",L"PayPal",L"PayPal",L"PayPal",L"PayPal",L"PayPal",L"PayPal",L"PayPal"},
        {L"USDT",L"USDT",L"USDT",L"USDT",L"USDT",L"USDT",L"USDT",L"USDT",L"USDT",L"USDT",L"USDT"},
        {L"用微信「扫一扫」扫描赞赏码。",L"用微信「掃一掃」掃描讚賞碼。",L"Scan this appreciation code with WeChat to send a tip.",L"WeChat で応援コードを読み取ってください。",L"위챗으로 후원 코드를 스캔해 주세요.",L"Scanne den Trinkgeldcode mit WeChat.",L"Scannez ce code avec WeChat pour laisser un pourboire.",L"Escanea el código con WeChat para dejar una propina.",L"Escaneie o código com o WeChat para enviar uma gorjeta.",L"Отсканируйте код в WeChat, чтобы отправить чаевые.",L"Bahşiş göndermek için kodu WeChat ile tarayın."},
        {L"用 PayPal 扫码，或打开收款页。",L"用 PayPal 掃碼，或開啟收款頁面。",L"Scan with PayPal or open the payment page.",L"PayPal でコードを読み取るか、支払いページを開いてください。",L"PayPal에서 코드를 스캔하거나 결제 페이지를 여세요.",L"Scanne den Code mit PayPal oder öffne die Zahlungsseite.",L"Scannez avec PayPal ou ouvrez la page de paiement.",L"Escanea con PayPal o abre la página de pago.",L"Escaneie com o PayPal ou abra a página de pagamento.",L"Отсканируйте код в PayPal или откройте страницу оплаты.",L"PayPal ile tarayın veya ödeme sayfasını açın."},
        {L"请仅通过 TRON（TRC20）网络发送 USDT。",L"請僅透過 TRON（TRC20）網路傳送 USDT。",L"Send USDT only on the TRON (TRC20) network.",L"USDT は TRON（TRC20）ネットワークでのみ送金してください。",L"USDT는 TRON(TRC20) 네트워크로만 보내 주세요.",L"Sende USDT nur über das TRON-Netzwerk (TRC20).",L"Envoyez vos USDT uniquement sur le réseau TRON (TRC20).",L"Envía USDT solo por la red TRON (TRC20).",L"Envie USDT apenas pela rede TRON (TRC20).",L"Отправляйте USDT только через сеть TRON (TRC20).",L"USDT’yi yalnızca TRON (TRC20) ağı üzerinden gönderin."},
        {L"打开收款页",L"開啟收款頁面",L"Open payment page",L"支払いページを開く",L"결제 페이지 열기",L"Zahlungsseite öffnen",L"Ouvrir la page de paiement",L"Abrir página de pago",L"Abrir página de pagamento",L"Открыть страницу оплаты",L"Ödeme sayfasını aç"},
        {L"复制地址",L"複製地址",L"Copy address",L"アドレスをコピー",L"주소 복사",L"Adresse kopieren",L"Copier l’adresse",L"Copiar dirección",L"Copiar endereço",L"Скопировать адрес",L"Adresi kopyala"},
        {L"地址已复制",L"已複製地址",L"Address copied",L"コピーしました",L"주소 복사됨",L"Adresse kopiert",L"Adresse copiée",L"Dirección copiada",L"Endereço copiado",L"Адрес скопирован",L"Adres kopyalandı"},
        {L"关闭",L"關閉",L"Close",L"閉じる",L"닫기",L"Schließen",L"Fermer",L"Cerrar",L"Fechar",L"Закрыть",L"Kapat"},
        {L"复制失败。请选中地址后手动复制。",L"複製失敗。請選取地址後自行複製。",L"Copy failed. Select the address and copy it manually.",L"アドレスを選択して手動でコピーしてください。",L"주소를 복사하지 못했습니다. 주소를 선택해 직접 복사해 주세요.",L"Kopieren fehlgeschlagen. Markiere und kopiere die Adresse von Hand.",L"La copie a échoué. Sélectionnez l’adresse et copiez-la manuellement.",L"No se pudo copiar. Selecciona la dirección y cópiala manualmente.",L"Falha ao copiar. Selecione o endereço e copie-o manualmente.",L"Не удалось скопировать. Выделите адрес и скопируйте его вручную.",L"Kopyalanamadı. Adresi seçip elle kopyalayın."},
        {L"无法打开收款页。请用手机扫码。",L"無法開啟收款頁面。請用手機掃碼。",L"Couldn’t open the payment page. Scan the code with your phone.",L"支払いページを開けませんでした。スマートフォンでコードを読み取ってください。",L"결제 페이지를 열지 못했습니다. 휴대전화로 코드를 스캔해 주세요.",L"Die Zahlungsseite konnte nicht geöffnet werden. Scanne den Code mit deinem Handy.",L"Impossible d’ouvrir la page de paiement. Scannez le code avec votre téléphone.",L"No se pudo abrir la página. Escanea el código con el móvil.",L"Não foi possível abrir a página. Escaneie o código com o celular.",L"Не удалось открыть страницу оплаты. Отсканируйте код с телефона.",L"Ödeme sayfası açılamadı. Kodu telefonunuzla tarayın."},
        {L"无法显示收款码。",L"無法顯示收款碼。",L"QR code unavailable.",L"QR コードを表示できません。",L"QR 코드를 표시할 수 없습니다.",L"QR-Code nicht verfügbar.",L"Code QR indisponible.",L"Código QR no disponible.",L"Código QR indisponível.",L"Не удалось показать QR-код.",L"QR kodu gösterilemiyor."}
    };
    static_assert(std::size(labels)==static_cast<size_t>(Key::QrUnavailable)+1);
    static_assert(std::size(labels[0])==std::size(i18n::names));
    return labels[static_cast<size_t>(key)][i18n::language];
}
}
