


--dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON,                       // ��Ʋ���� ON
--dfMONITOR_DATA_TYPE_BATTLE_CPU,                             // ��Ʋ���� CPU ���� (Ŀ�� + ����)
--dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT,                   // ��Ʋ���� �޸� ���� Ŀ�� ��뷮 (Private) MByte
--dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL,                     // ��Ʋ���� ��ŶǮ ��뷮
--dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS,                        // ��Ʋ���� Auth ������ �ʴ� ���� ��
--dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS,                        // ��Ʋ���� Game ������ �ʴ� ���� ��
--dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL,                     // ��Ʋ���� ���� ������ü
--dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH,                    // ��Ʋ���� Auth ������ ��� �ο�
--dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME,                    // ��Ʋ���� Game ������ ��� �ο�
--dfMONITOR_DATA_TYPE_BATTLE_,		                        // ��Ʋ���� �α����� ������ ��ü �ο�		<- ����
--dfMONITOR_DATA_TYPE_BATTLE_ROOM_WAIT,                       // ��Ʋ���� ���� ��
--dfMONITOR_DATA_TYPE_BATTLE_ROOM_PLAY,                       // ��Ʋ���� �÷��̹� ��

CREATE DATABASE `status_server`;

CREATE TABLE `status_server`.`hardware` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
	`date` datetime NOT NULL,
	`CPUUsage` BIGINT NOT NULL,
	`AvailableMBytes` BIGINT NOT NULL,
	`NonPagedBytes` BIGINT NOT NULL,
	PRIMARY KEY (`no`),
	INDEX `date_INDEX` (`date` ASC)
);

CREATE TABLE `status_server`.`battle` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
	`date` datetime NOT NULL,
	`CPUUsage` BIGINT NOT NULL,
	`CommitMemory` BIGINT NOT NULL,
	`PacketPool` BIGINT NOT NULL,
	`AuthFPS` BIGINT NOT NULL,
	`GameFPS` BIGINT NOT NULL,
	`SessionAll` BIGINT NOT NULL,
	`SessionAuth` BIGINT NOT NULL,
	`SessionGame` BIGINT NOT NULL,
	PRIMARY KEY (`no`),
	INDEX `date_INDEX` (`date` ASC)
);

--INSERT INTO `status_server`.`battle` (`date`, `CPUUsage`, `CommitMemory`, `PacketPool`, `AuthFPS`, `GameFPS`, `SessionAll`, `SessionAuth`, `SessionGame`) VALUES (NOW(), '1', '1', '1', '1', '1', '1', '1', '1');

CREATE TABLE `status_server`.`chat` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
	`date` datetime NOT NULL,
	`CPUUsage` BIGINT NOT NULL,
	`CommitMemory` BIGINT NOT NULL,
	`PacketPool` BIGINT NOT NULL,
	`SessionAll` BIGINT NOT NULL,
	`SessionLogin` BIGINT NOT NULL,
	PRIMARY KEY (`no`),
	INDEX `date_INDEX` (`date` ASC)
);

--INSERT INTO `status_server`.`chat` (`date`, `CPUUsage`, `CommitMemory`, `PacketPool`, `SessionAll`, `SessionLogin`) VALUES (NOW(), '1', '1', '1', '1', '1');

CREATE TABLE `status_server`.`login` (
	`no` BIGINT NOT NULL AUTO_INCREMENT,
	`date` datetime NOT NULL,
	`CPUUsage` BIGINT NOT NULL,
	`CommitMemory` BIGINT NOT NULL,
	`PacketPool` BIGINT NOT NULL,
	`SessionAll` BIGINT NOT NULL,
	`LoginSuccessTPS` BIGINT NOT NULL,
	PRIMARY KEY (`no`),
	INDEX `date_INDEX` (`date` ASC)
);

--INSERT INTO `status_server`.`login` (`date`, `CPUUsage`, `CommitMemory`, `PacketPool`, `SessionAll`, `LoginSuccessTPS`) VALUES (NOW(), '1', '1', '1', '1', '1');





