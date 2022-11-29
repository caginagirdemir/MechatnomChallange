# MechatnomChallange

Main içinde 2 task çalışacak. (cooperative multitasking)
1. task led on off yapacak. Örnek (300ms ON 700ms off) on off süreleri ayarlanabilir olacak. 
Nasıl ayarlanacağını aşağıdadır.    
2. task uart'tan gelen datayı aynı port üzerinden echo yapacak. Uart interrupt ile çalışacak ve ayarları değiştirilebilir olacak. default 115200 8N1
Uart'tan "stop" stringi gönderdiğimizde echo taskı sonlanacak ve led 1sn aralıklarla yanıp sönecek.
Uart'tan "start" stringi gönderdiğimizde echo taskı yeniden başlayacak ve led bir önceki ayarlarla çalışmaya devam edecek. 
Uart'tan "ledon=500" yazarsak led on süresi 500 ms olacak.
Uart'tan "ledoff=500" yazarsak led off süresi 500 ms olacak.
Bu 2 task birbirini bloke etmeyecek.
