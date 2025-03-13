const WebSocket = require('ws');
const url = require('url');

const wss = new WebSocket.Server({ port: 8080 });

console.log("WebSocket server 8080-portda ishlayapti...");

wss.on('connection', (ws, req) => {
    const pathname = url.parse(req.url).pathname;

    if (pathname !== "/car_washing") {
        console.log(`❌ Noto‘g‘ri endpoint: ${pathname}, ulanish rad etildi.`);
        ws.close();
        return;
    }

    console.log("✅ Yangi mijoz /car_washing endpointiga ulandi");

    ws.on('message', (message) => {
        console.log("📩 Mijozdan xabar keldi:", message.toString());
    });

    ws.on('close', () => {
        console.log("❌ Mijoz uzildi");
    });
});

// Mijozlarga xabar yuborish funksiyasi
function sendToAllClients(amount) {
    const jsonData = JSON.stringify({
        action: "cash",
        washId: 9,
        terminalId: 9,
        amount: amount
    });

    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(jsonData);
            console.log(`📤 Yuborildi: ${jsonData}`);
        }
    });
}

// Har 10 sekundda 5000 yuborish
// setInterval(() => {
//     sendToAllClients(5000);
// }, 20000);
// // Har 20 sekundda 10000 yuborish
// setInterval(() => {
//     sendToAllClients(10000);
// }, 20000);
