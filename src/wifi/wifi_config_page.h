#pragma once
const char WIFI_CONFIG_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>大菠萝车机 WiFi配置</title>
    <style>
        body { 
            font-family: Arial, sans-serif;
            padding: 20px;
            background-color: #f5f5f5;
            max-width: 600px;
            margin: 0 auto;
        }
        .container {
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        }
        .logo {
            text-align: center;
            margin-bottom: 20px;
        }
        .logo h1 {
            color: #333;
            font-size: 24px;
        }
        .logo span {
            color: #666;
            font-size: 14px;
        }
        input {
            width: 100%;
            padding: 10px;
            margin: 10px 0;
            border: 1px solid #ddd;
            border-radius: 5px;
            box-sizing: border-box;
        }
        button {
            width: 100%;
            padding: 12px;
            background-color: #007bff;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
            margin-top: 15px;
        }
        button:hover {
            background-color: #0056b3;
        }
        .footer {
            text-align: center;
            margin-top: 20px;
            color: #666;
            font-size: 12px;
        }
        .error {
            color: red;
            text-align: center;
            margin-bottom: 10px;
        }
        .sleep-info {
            margin: 20px 0; text-align: center;
        }
        .sleep-countdown {
            font-size: 16px; color: #333; margin-right: 10px;
        }
        .saved-wifi-list {
            margin-top: 30px;
        }
        .saved-wifi-list h3 {
            margin-bottom: 10px;
        }
        .saved-wifi-list ul {
            list-style: none;
            padding: 0;
        }
        .saved-wifi-list li {
            margin-bottom: 8px;
        }
        .saved-wifi-list li span {
            margin-right: 10px;
        }
        .saved-wifi-list li button {
            padding: 2px 8px;
            font-size: 13px;
        }
    </style>
    <script>
    // 动态获取热点列表并填充 datalist
    function fetchWifiList() {
        fetch('/scan')
            .then(response => response.json())
            .then(data => {
                let list = document.getElementById('wifi-list');
                list.innerHTML = '';
                data.forEach(ssid => {
                    let option = document.createElement('option');
                    option.value = ssid;
                    list.appendChild(option);
                });
            });
    }
    function renderSavedWifi() {
        fetch('/saved_wifi')
            .then(r => r.json())
            .then(list => {
                let ul = document.getElementById('saved-wifi-ul');
                ul.innerHTML = '';
                if(list.length === 0) {
                    ul.innerHTML = '<li style="color:#888;">暂无已保存WiFi</li>';
                } else {
                    list.forEach(item => {
                        let li = document.createElement('li');
                        li.style.marginBottom = '8px';
                        li.innerHTML = `<span style='margin-right:10px;'>${item.ssid}</span><button style='padding:2px 8px;font-size:13px;' onclick="deleteWifi('${item.ssid}')">删除</button>`;
                        ul.appendChild(li);
                    });
                }
            });
    }
    function deleteWifi(ssid) {
        fetch('/delete_wifi', {
            method: 'POST',
            headers: {'Content-Type': 'application/x-www-form-urlencoded'},
            body: 'ssid=' + encodeURIComponent(ssid)
        }).then(() => renderSavedWifi());
    }
    function exitConfig() {
        fetch('/exit_config', {method: 'POST'})
            .then(() => {
                alert('已退出配网模式，设备将尝试连接WiFi');
            });
    }
    window.onload = function() {
        fetchWifiList();
        renderSavedWifi();
    };
    </script>
</head>
<body>
    <div class="container">
        <div class="logo">
            <h1>大菠萝车机</h1>
            <span>智能摩托车数据盒子</span>
        </div>
        <div class="sleep-info">
        </div>
        <form method='post' action='/configure'>
            <input list='wifi-list' name='ssid' placeholder='选择或输入WiFi名称' required>
            <datalist id='wifi-list'></datalist>
            <input type='password' name='password' placeholder='请输入WiFi密码' required>
            <button type='submit'>连接网络</button>
        </form>
        <button type="button" onclick="exitConfig()" style="margin-top:15px;background:#dc3545;">退出配网模式</button>
        <div class="saved-wifi-list" style="margin-top:30px;">
            <h3>已保存的WiFi</h3>
            <ul id="saved-wifi-ul" style="list-style:none;padding:0;"></ul>
        </div>
        <div class="footer">
            <p>© 2024 大菠萝车机. 请连接到您的WiFi网络以继续使用。</p>
        </div>
    </div>
</body>
</html>
)rawliteral";
