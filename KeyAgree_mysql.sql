/*
Navicat MySQL Data Transfer

Source Server         : 192.168.23.571
Source Server Version : 50620
Source Host           : 192.168.1.100:3306
Source Database       : key_agreement_system

Target Server Type    : MYSQL
Target Server Version : 50620
File Encoding         : 65001

Date: 2021-04-12 16:53:50
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- 创建 数据库 并 选择
-- ----------------------------

CREATE DATABASE key_agreement_system; 
USE key_agreement_system;

-- ----------------------------
-- keysn序列号
-- ----------------------------
DROP TABLE IF EXISTS `keysn`;
CREATE TABLE `keysn` (
  `ikeysn` int(12) NOT NULL,
  PRIMARY KEY (`ikeysn`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of keysn
-- ----------------------------
INSERT INTO `keysn` VALUES ('1');

-- -----------------------------------------------------------------------------
-- -- 创建 网点密钥表, 客户端网点 服务器端网点 密钥号 密钥产生时间 密钥状态 
-- -----------------------------------------------------------------------------
DROP TABLE IF EXISTS `seckeyinfo`;
CREATE TABLE `seckeyinfo` (
  `clientid` int(4) DEFAULT NULL,
  `serverid` int(4) DEFAULT NULL,
  `keyid` int(9) NOT NULL AUTO_INCREMENT,
  `createtime` date DEFAULT NULL,
  `state` int(4) DEFAULT NULL,
  `seckey` varchar(512) DEFAULT NULL,
  PRIMARY KEY (`keyid`),
  KEY `index_serverid` (`serverid`),
  KEY `index_clientid` (`clientid`),
  CONSTRAINT `seckeynode_clientid_fk` FOREIGN KEY (`clientid`) REFERENCES `secnode` (`id`),
  CONSTRAINT `seckeynode_serverid_fk` FOREIGN KEY (`serverid`) REFERENCES `secnode` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of seckeyinfo
-- ----------------------------

-- ---------------------------------------------------------------
-- 创建 网点信息表 --编号 名称 描述 授权码 状态(0可用  1不可用)
-- ---------------------------------------------------------------
DROP TABLE IF EXISTS `secnode`;
CREATE TABLE `secnode` (
  `id` int(4) NOT NULL AUTO_INCREMENT,
  `name` varchar(512) NOT NULL,
  `nodedesc` varchar(512) DEFAULT NULL,
  `createtime` date DEFAULT NULL,
  `authcode` int(12) DEFAULT NULL,
  `state` int(4) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of secnode
-- ----------------------------
INSERT INTO secnode(name,nodedesc,createtime,authcode,state) VALUES ('服务端', '和平区', '2021-04-12', '0', '0');

INSERT INTO secnode(name,nodedesc,createtime,authcode,state) VALUES ('客户端1', '铁西区', '2021-04-12', '0', '0');

INSERT INTO secnode(name,nodedesc,createtime,authcode,state) VALUES ('客户端2', '沈河区', '2021-04-12', '0', '0');

-- ----------------------------
-- Table structure for srvcfg
-- ----------------------------
DROP TABLE IF EXISTS `srvcfg`;
CREATE TABLE `srvcfg` (
  `key` varchar(64) DEFAULT NULL,
  `valude` varchar(128) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of srvcfg
-- ----------------------------
INSERT INTO `srvcfg` VALUES ('secmng_server_ip', '192.168.1.1');

SET FOREIGN_KEY_CHECKS=1;
