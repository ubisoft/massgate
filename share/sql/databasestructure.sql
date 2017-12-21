SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `live`
--
CREATE DATABASE IF NOT EXISTS live;
USE live;

GRANT ALL ON live.* TO 'massgateadmin'@'localhost' IDENTIFIED BY '8fesfsdBOrwe';
GRANT SELECT ON live.* TO 'massgateclient'@'localhost' IDENTIFIED BY 'po389ef0sS';
FLUSH PRIVILEGES;

-- --------------------------------------------------------

--
-- Table structure for table `3300_8600_to_11900`
--

DROP TABLE IF EXISTS `3300_8600_to_11900`;
CREATE TABLE IF NOT EXISTS `3300_8600_to_11900` (
  `banned` varchar(32) default NULL,
  `generated` varchar(32) default NULL,
  `used` enum('n','y') default 'n'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `AccessCodeBatches`
--

DROP TABLE IF EXISTS `AccessCodeBatches`;
CREATE TABLE IF NOT EXISTS `AccessCodeBatches` (
  `batchId` int(10) unsigned NOT NULL auto_increment,
  `creationDate` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `identifier` varchar(255) NOT NULL default '',
  `numCodesInBatch` int(10) unsigned NOT NULL default '0',
  `codeType` mediumint(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`batchId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `AccessCodes`
--

DROP TABLE IF EXISTS `AccessCodes`;
CREATE TABLE IF NOT EXISTS `AccessCodes` (
  `sequenceNumber` int(10) unsigned NOT NULL default '0',
  `secretCode` varchar(5) NOT NULL default '',
  `productId` mediumint(8) unsigned NOT NULL default '0',
  `accessCode` varchar(16) NOT NULL default '',
  `dateAdded` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `batchId` int(10) unsigned NOT NULL default '0',
  `codeType` mediumint(8) unsigned NOT NULL default '0',
  `claimedBy` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`sequenceNumber`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Accounts`
--

DROP TABLE IF EXISTS `Accounts`;
CREATE TABLE IF NOT EXISTS `Accounts` (
  `accountid` int(10) unsigned NOT NULL auto_increment,
  `password` varchar(128) NOT NULL default '',
  `email` varchar(64) NOT NULL default '',
  `country` varchar(4) NOT NULL default '',
  `acceptsWicEmail` tinyint(4) NOT NULL default '0',
  `acceptsSierraEmail` tinyint(4) NOT NULL default '0',
  `isBanned` int(11) NOT NULL default '0',
  `dateAdded` datetime NOT NULL default '0000-00-00 00:00:00',
  `createdFromCdKey` int(10) unsigned NOT NULL default '0',
  `groupMembership` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`email`),
  UNIQUE KEY `userid` (`accountid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Acquaintances`
--

DROP TABLE IF EXISTS `Acquaintances`;
CREATE TABLE IF NOT EXISTS `Acquaintances` (
  `lowerProfileId` int(10) unsigned NOT NULL default '0',
  `higherProfileId` int(10) unsigned NOT NULL default '0',
  `numTimesPlayed` int(10) unsigned NOT NULL default '0',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  UNIQUE KEY `acquaintance` (`lowerProfileId`,`higherProfileId`),
  KEY `higherProfileId` (`higherProfileId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Backup_ClanMatchLog`
--

DROP TABLE IF EXISTS `Backup_ClanMatchLog`;
CREATE TABLE IF NOT EXISTS `Backup_ClanMatchLog` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `winner` int(10) unsigned NOT NULL default '0',
  `loser` int(10) unsigned NOT NULL default '0',
  `winnerScore` int(11) NOT NULL default '0',
  `loserScore` int(11) NOT NULL default '0',
  `added` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `crapData` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `BannedStrings`
--

DROP TABLE IF EXISTS `BannedStrings`;
CREATE TABLE IF NOT EXISTS `BannedStrings` (
  `isName` tinyint(1) NOT NULL default '0',
  `isChatWord` tinyint(1) NOT NULL default '0',
  `isServerName` tinyint(1) NOT NULL default '0',
  `string` varchar(64) NOT NULL default '',
  PRIMARY KEY  (`string`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Banners`
--

DROP TABLE IF EXISTS `Banners`;
CREATE TABLE IF NOT EXISTS `Banners` (
  `bannerId` int(11) NOT NULL auto_increment,
  `supplierId` int(10) unsigned default NULL,
  `url` varchar(255) default NULL,
  `hash` bigint(20) unsigned default NULL,
  `revoked` tinyint(1) default '0',
  `impressions` int(10) unsigned default '0',
  `type` int(3) unsigned default '0',
  PRIMARY KEY  (`bannerId`),
  KEY `hash` (`hash`),
  KEY `supplierId` (`supplierId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `BestOfLadder`
--

DROP TABLE IF EXISTS `BestOfLadder`;
CREATE TABLE IF NOT EXISTS `BestOfLadder` (
  `id` int(11) NOT NULL auto_increment,
  `profileId` int(11) NOT NULL default '0',
  `score` int(11) NOT NULL default '0',
  `entered` datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `BlackListedMaps`
--

DROP TABLE IF EXISTS `BlackListedMaps`;
CREATE TABLE IF NOT EXISTS `BlackListedMaps` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `mapHash` bigint(20) unsigned NOT NULL default '0',
  `dateAdded` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `mapName` varchar(64) NOT NULL default '',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Cafe_Info`
--

DROP TABLE IF EXISTS `Cafe_Info`;
CREATE TABLE IF NOT EXISTS `Cafe_Info` (
  `id` int(11) NOT NULL auto_increment,
  `email` varchar(255) NOT NULL default '',
  `cdkey` int(11) NOT NULL default '0',
  `currentConcur` int(11) NOT NULL default '0',
  `maxConcur` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `email` (`email`),
  UNIQUE KEY `cdKey` (`cdkey`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Cafe_Log`
--

DROP TABLE IF EXISTS `Cafe_Log`;
CREATE TABLE IF NOT EXISTS `Cafe_Log` (
  `id` int(11) NOT NULL auto_increment,
  `cafeId` int(11) NOT NULL default '0',
  `action` enum('session','to_few_keys') NOT NULL default 'session',
  `entered` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `duration` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Cafe_ShoppingHistory`
--

DROP TABLE IF EXISTS `Cafe_ShoppingHistory`;
CREATE TABLE IF NOT EXISTS `Cafe_ShoppingHistory` (
  `id` int(11) NOT NULL auto_increment,
  `cafeId` int(11) NOT NULL default '0',
  `entered` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `bought` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Cafe_WhiteList`
--

DROP TABLE IF EXISTS `Cafe_WhiteList`;
CREATE TABLE IF NOT EXISTS `Cafe_WhiteList` (
  `id` int(11) NOT NULL auto_increment,
  `cafeId` int(11) NOT NULL default '0',
  `ip` varchar(32) default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `cafeIdIp` (`cafeId`,`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `CdKeyBatches`
--

DROP TABLE IF EXISTS `CdKeyBatches`;
CREATE TABLE IF NOT EXISTS `CdKeyBatches` (
  `batchId` int(10) unsigned NOT NULL auto_increment,
  `creationDate` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `identifier` varchar(255) NOT NULL default '',
  `numKeysInBatch` int(10) unsigned NOT NULL default '0',
  `productId` mediumint(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`batchId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

INSERT INTO `CdKeyBatches` (`batchId`, `numKeysInBatch`, `productId`) VALUES
('1', '2', '3');

-- --------------------------------------------------------

--
-- Table structure for table `CdKeys`
--

DROP TABLE IF EXISTS `CdKeys`;
CREATE TABLE IF NOT EXISTS `CdKeys` (
  `sequenceNumber` int(10) unsigned NOT NULL default '0',
  `cdKey` varchar(40) NOT NULL default '',
  `isBanned` tinyint(4) NOT NULL default '0',
  `isGuestKey` tinyint(3) unsigned NOT NULL default '0',
  `batchId` int(10) unsigned NOT NULL default '0',
  `numAccountCreations` int(10) unsigned NOT NULL default '0',
  `maxAccountCreations` int(10) unsigned NOT NULL default '1',
  `groupMembership` int(10) unsigned NOT NULL default '0',
  `activationDate` datetime NOT NULL default '0000-00-00 00:00:00',
  `leaseTime` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`sequenceNumber`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

INSERT INTO `CdKeys` (`sequenceNumber`, `cdKey`, `batchId`, `numAccountCreations`, `maxAccountCreations`, `groupMembership`) VALUES
('1', 'S4AKHJULTX8MWOXVIMGS46FVEC', '1', '0', '2147483647', '255'),
('5786378', 'YSWIPAJOR7WWW8LEFOP4T6KUYF', '1', '0', '2147483647', '255');

-- --------------------------------------------------------

--
-- Table structure for table `ClanAntiSmurfer`
--

DROP TABLE IF EXISTS `ClanAntiSmurfer`;
CREATE TABLE IF NOT EXISTS `ClanAntiSmurfer` (
  `id` int(11) NOT NULL auto_increment,
  `accountId` int(11) NOT NULL default '0',
  `profileId` int(11) NOT NULL default '0',
  `clanId` int(11) NOT NULL default '0',
  `lastMatch` timestamp NOT NULL default CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `accountId` (`accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `ClanGuestbook`
--

DROP TABLE IF EXISTS `ClanGuestbook`;
CREATE TABLE IF NOT EXISTS `ClanGuestbook` (
  `id` int(11) NOT NULL auto_increment,
  `clanId` int(10) unsigned NOT NULL default '0',
  `posterProfileId` int(10) unsigned NOT NULL default '0',
  `timestamp` datetime NOT NULL default '0000-00-00 00:00:00',
  `message` varchar(128) NOT NULL default '',
  `isDeleted` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `clanIdIndex` (`clanId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `ClanInvitations`
--

DROP TABLE IF EXISTS `ClanInvitations`;
CREATE TABLE IF NOT EXISTS `ClanInvitations` (
  `invitationId` int(10) unsigned NOT NULL auto_increment,
  `profileId` int(10) unsigned NOT NULL default '0',
  `clanId` int(10) unsigned NOT NULL default '0',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`invitationId`),
  UNIQUE KEY `profileId` (`profileId`,`clanId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `ClanLadder`
--

DROP TABLE IF EXISTS `ClanLadder`;
CREATE TABLE IF NOT EXISTS `ClanLadder` (
  `id` int(10) unsigned NOT NULL default '0',
  `gracePeriodEnd` datetime default NULL,
  `rating` int(11) default NULL,
  `deviation` int(11) default NULL,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `ClanMatchLog`
--

DROP TABLE IF EXISTS `ClanMatchLog`;
CREATE TABLE IF NOT EXISTS `ClanMatchLog` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `winner` int(10) unsigned NOT NULL default '0',
  `loser` int(10) unsigned NOT NULL default '0',
  `winnerScore` int(11) NOT NULL default '0',
  `loserScore` int(11) NOT NULL default '0',
  `winnerFaction` tinyint(3) unsigned NOT NULL default '0',
  `loserFaction` tinyint(3) unsigned NOT NULL default '0',
  `added` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `mapHash` bigint(20) unsigned NOT NULL default '0',
  `mapName` varchar(255) NOT NULL default '',
  `winnerCS` int(10) unsigned NOT NULL default '0',
  `loserCS` int(10) unsigned NOT NULL default '0',
  `gameMode` tinyint(3) unsigned NOT NULL default '0',
  `timeUsed` int(10) unsigned NOT NULL default '0',
  `winnerGameData1` int(11) NOT NULL default '0',
  `winnerGameData2` int(11) NOT NULL default '0',
  `loserGameData1` int(11) NOT NULL default '0',
  `loserGameData2` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `winner` (`winner`),
  KEY `loser` (`loser`),
  KEY `added` (`added`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `ClanMatchLogPerPlayer`
--

DROP TABLE IF EXISTS `ClanMatchLogPerPlayer`;
CREATE TABLE IF NOT EXISTS `ClanMatchLogPerPlayer` (
  `id` int(11) NOT NULL auto_increment,
  `matchId` int(10) unsigned NOT NULL default '0',
  `wasWinner` tinyint(1) unsigned NOT NULL default '0',
  `profileId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  `role` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `matchId` (`matchId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `ClanMedals`
--

DROP TABLE IF EXISTS `ClanMedals`;
CREATE TABLE IF NOT EXISTS `ClanMedals` (
  `clanId` int(10) unsigned NOT NULL default '0',
  `winStreak` tinyint(1) unsigned NOT NULL default '0',
  `winStreakStars` tinyint(1) unsigned NOT NULL default '0',
  `domSpec` tinyint(1) unsigned NOT NULL default '0',
  `domSpecStars` tinyint(1) unsigned NOT NULL default '0',
  `domEx` tinyint(1) unsigned NOT NULL default '0',
  `domExStars` tinyint(1) unsigned NOT NULL default '0',
  `assSpec` tinyint(1) unsigned NOT NULL default '0',
  `assSpecStars` tinyint(1) unsigned NOT NULL default '0',
  `assEx` tinyint(1) unsigned NOT NULL default '0',
  `assExStars` tinyint(1) unsigned NOT NULL default '0',
  `towSpec` tinyint(1) unsigned NOT NULL default '0',
  `towSpecStars` tinyint(1) unsigned NOT NULL default '0',
  `towEx` tinyint(1) unsigned NOT NULL default '0',
  `towExStars` tinyint(1) unsigned NOT NULL default '0',
  `hsAwd` tinyint(1) unsigned NOT NULL default '0',
  `hsAwdStars` tinyint(1) unsigned NOT NULL default '0',
  `tcAwd` tinyint(1) unsigned NOT NULL default '0',
  `tcAwdStars` tinyint(1) unsigned NOT NULL default '0',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`clanId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Clans`
--

DROP TABLE IF EXISTS `Clans`;
CREATE TABLE IF NOT EXISTS `Clans` (
  `clanId` int(10) unsigned NOT NULL auto_increment,
  `fullName` varchar(32) NOT NULL default '',
  `shortName` varchar(8) NOT NULL default '',
  `tagFormat` varchar(8) NOT NULL default '',
  `motto` varchar(128) NOT NULL default '',
  `leaderSays` varchar(128) NOT NULL default '',
  `homepage` varchar(128) NOT NULL default '',
  `playerOfWeek` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`clanId`),
  UNIQUE KEY `shortName` (`shortName`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `ClanStats`
--

DROP TABLE IF EXISTS `ClanStats`;
CREATE TABLE IF NOT EXISTS `ClanStats` (
  `clanId` int(10) unsigned NOT NULL default '0',
  `n_matches` int(10) unsigned NOT NULL default '0',
  `n_matcheswon` int(10) unsigned NOT NULL default '0',
  `n_bestmatchstreak` int(10) unsigned NOT NULL default '0',
  `n_currentmatchstreak` int(10) unsigned NOT NULL default '0',
  `n_toursplayed` int(10) unsigned NOT NULL default '0',
  `n_tourswon` int(10) unsigned NOT NULL default '0',
  `n_dommatch` int(10) unsigned NOT NULL default '0',
  `n_dommatchwon` int(10) unsigned NOT NULL default '0',
  `n_currentdomstreak` int(10) unsigned NOT NULL default '0',
  `n_dommatchwbtd` int(10) unsigned NOT NULL default '0',
  `n_dommatchwbtdstreak` int(10) unsigned NOT NULL default '0',
  `n_assmatch` int(10) unsigned NOT NULL default '0',
  `n_assmatchwon` int(10) unsigned NOT NULL default '0',
  `n_currentassstreak` int(10) unsigned NOT NULL default '0',
  `n_assmatchpd` int(10) unsigned NOT NULL default '0',
  `n_assmatchpdstreak` int(10) unsigned NOT NULL default '0',
  `n_towmatch` int(10) unsigned NOT NULL default '0',
  `n_towmatchwon` int(10) unsigned NOT NULL default '0',
  `n_currenttowstreak` int(10) unsigned NOT NULL default '0',
  `n_towmatchpp` int(10) unsigned NOT NULL default '0',
  `n_towmatchppstreak` int(10) unsigned NOT NULL default '0',
  `n_unitslost` int(10) unsigned NOT NULL default '0',
  `n_unitskilled` int(10) unsigned NOT NULL default '0',
  `n_nukesdep` int(10) unsigned NOT NULL default '0',
  `n_tacrithits` int(10) unsigned NOT NULL default '0',
  `t_playingAsUSA` int(10) unsigned NOT NULL default '0',
  `t_playingAsUSSR` int(10) unsigned NOT NULL default '0',
  `t_playingAsNATO` int(10) unsigned NOT NULL default '0',
  `s_scoreTotal` int(10) unsigned NOT NULL default '0',
  `s_scoreByDamagingEnemies` int(10) unsigned NOT NULL default '0',
  `s_scoreByRepairing` int(10) unsigned NOT NULL default '0',
  `s_scoreByTransporting` int(10) unsigned NOT NULL default '0',
  `s_scoreByTacticalAid` int(10) unsigned NOT NULL default '0',
  `s_scoreByFortifying` int(10) unsigned NOT NULL default '0',
  `s_scoreByArmor` int(10) unsigned NOT NULL default '0',
  `s_scoreByAir` int(10) unsigned NOT NULL default '0',
  `s_scoreBySupport` int(10) unsigned NOT NULL default '0',
  `s_scoreByInfantry` int(10) unsigned NOT NULL default '0',
  `s_highestAsInfantry` int(10) unsigned NOT NULL default '0',
  `s_highestAsSupport` int(10) unsigned NOT NULL default '0',
  `s_highestAsAir` int(10) unsigned NOT NULL default '0',
  `s_highestAsArmor` int(10) unsigned NOT NULL default '0',
  `s_highestScoreInAMatch` int(10) unsigned NOT NULL default '0',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`clanId`),
  KEY `clanId` (`clanId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `ClanStatsTweakables`
--

DROP TABLE IF EXISTS `ClanStatsTweakables`;
CREATE TABLE IF NOT EXISTS `ClanStatsTweakables` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `aKey` varchar(32) NOT NULL default '',
  `aValue` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `ClientMetrics`
--

DROP TABLE IF EXISTS `ClientMetrics`;
CREATE TABLE IF NOT EXISTS `ClientMetrics` (
  `accountId` int(10) unsigned NOT NULL default '0',
  `k` varchar(128) character set utf8 NOT NULL default '',
  `v` varchar(96) character set utf8 NOT NULL default '',
  `min_val` varchar(20) character set utf8 NOT NULL default '',
  `max_val` varchar(20) character set utf8 NOT NULL default '',
  `sum_val` varchar(20) character set utf8 NOT NULL default '',
  `num_val` int(10) unsigned NOT NULL default '1',
  `context` tinyint(3) unsigned NOT NULL default '0',
  `lastChanged` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  UNIQUE KEY `accountId` (`accountId`,`k`,`context`),
  KEY `k` (`k`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `ClientStrings`
--

DROP TABLE IF EXISTS `ClientStrings`;
CREATE TABLE IF NOT EXISTS `ClientStrings` (
  `s2i` int(10) unsigned NOT NULL default '0',
  `str` varchar(128) NOT NULL default '',
  UNIQUE KEY `s2i` (`s2i`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Countries`
--

DROP TABLE IF EXISTS `Countries`;
CREATE TABLE IF NOT EXISTS `Countries` (
  `cc` varchar(4) character set utf8 NOT NULL default '',
  `cn` varchar(64) character set utf8 NOT NULL default '',
  PRIMARY KEY  (`cc`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `DebugUploadTickets`
--

DROP TABLE IF EXISTS `DebugUploadTickets`;
CREATE TABLE IF NOT EXISTS `DebugUploadTickets` (
  `ticketId` int(10) unsigned NOT NULL auto_increment,
  `description` varchar(255) NOT NULL default '',
  `callstackHash` bigint(20) unsigned NOT NULL default '0',
  `cdkey` int(10) unsigned NOT NULL default '0',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`ticketId`),
  KEY `lastUpdated` (`lastUpdated`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Distributions`
--

DROP TABLE IF EXISTS `Distributions`;
CREATE TABLE IF NOT EXISTS `Distributions` (
  `distributionid` int(11) unsigned NOT NULL auto_increment,
  `productid` int(11) unsigned NOT NULL default '0',
  `distribution` varchar(32) NOT NULL default '',
  `latestversion` varchar(16) NOT NULL default '',
  PRIMARY KEY  (`distributionid`,`productid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `foobar`
--

DROP TABLE IF EXISTS `foobar`;
CREATE TABLE IF NOT EXISTS `foobar` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `playerId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `playerId` (`playerId`),
  KEY `playerId_2` (`playerId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Friends`
--

DROP TABLE IF EXISTS `Friends`;
CREATE TABLE IF NOT EXISTS `Friends` (
  `subjectProfileId` int(10) unsigned NOT NULL default '0',
  `objectProfileId` int(10) unsigned NOT NULL default '0',
  UNIQUE KEY `subjectProfileId` (`subjectProfileId`,`objectProfileId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `GeoIP`
--

DROP TABLE IF EXISTS `GeoIP`;
CREATE TABLE IF NOT EXISTS `GeoIP` (
  `start_ip` varchar(15) NOT NULL default '',
  `end_ip` varchar(15) NOT NULL default '',
  `start` int(10) unsigned NOT NULL default '0',
  `end` int(10) unsigned NOT NULL default '0',
  `cc` char(2) NOT NULL default '',
  `cn` varchar(50) NOT NULL default '',
  UNIQUE KEY `start` (`start`),
  KEY `end` (`end`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `GuestKeyIpWhiteList`
--

DROP TABLE IF EXISTS `GuestKeyIpWhiteList`;
CREATE TABLE IF NOT EXISTS `GuestKeyIpWhiteList` (
  `id` int(11) NOT NULL auto_increment,
  `cdKeySeqNum` int(10) unsigned NOT NULL default '0',
  `ip` varchar(24) NOT NULL default '',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `seqNumIp` (`cdKeySeqNum`,`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `ImageHosting`
--

DROP TABLE IF EXISTS `ImageHosting`;
CREATE TABLE IF NOT EXISTS `ImageHosting` (
  `id` int(11) NOT NULL auto_increment,
  `inUse` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `InstantMessageAccountSettings`
--

DROP TABLE IF EXISTS `InstantMessageAccountSettings`;
CREATE TABLE IF NOT EXISTS `InstantMessageAccountSettings` (
  `accountId` int(10) unsigned NOT NULL default '0',
  `friends` smallint(5) unsigned NOT NULL default '7',
  `clanmembers` smallint(5) unsigned NOT NULL default '7',
  `acquaintances` smallint(5) unsigned NOT NULL default '7',
  `anyone` smallint(5) unsigned NOT NULL default '6',
  UNIQUE KEY `accountId` (`accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='bitfield: ReceiveAtAll,ReceiveWhilePlaying,ReceiveAsEmail';

-- --------------------------------------------------------

--
-- Table structure for table `InstantMessages`
--

DROP TABLE IF EXISTS `InstantMessages`;
CREATE TABLE IF NOT EXISTS `InstantMessages` (
  `messageId` int(10) unsigned NOT NULL auto_increment,
  `senderProfileId` int(10) unsigned NOT NULL default '0',
  `recipientProfileId` int(10) unsigned NOT NULL default '0',
  `messageText` varchar(255) NOT NULL default '',
  `writtenAt` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `sendAsEmail` enum('NO','YES','SENT') character set latin1 NOT NULL default 'NO',
  `validUntil` timestamp NOT NULL default '0000-00-00 00:00:00',
  `isRead` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`messageId`),
  KEY `sendAsEmail` (`sendAsEmail`),
  KEY `recipientProfileId` (`recipientProfileId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `IpAllowPatch`
--

DROP TABLE IF EXISTS `IpAllowPatch`;
CREATE TABLE IF NOT EXISTS `IpAllowPatch` (
  `cnet` varchar(16) NOT NULL default '',
  `description` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`cnet`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Ipbans`
--

DROP TABLE IF EXISTS `Ipbans`;
CREATE TABLE IF NOT EXISTS `Ipbans` (
  `id` int(11) NOT NULL auto_increment,
  `ip` varchar(64) NOT NULL default '',
  `entered` datetime NOT NULL default '0000-00-00 00:00:00',
  `expires` datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Log_Abuse`
--

DROP TABLE IF EXISTS `Log_Abuse`;
CREATE TABLE IF NOT EXISTS `Log_Abuse` (
  `abuseId` int(10) unsigned NOT NULL auto_increment,
  `reporterAccountId` int(10) unsigned NOT NULL default '0',
  `reporterProfileId` int(10) unsigned NOT NULL default '0',
  `reporteeAccountId` int(10) unsigned NOT NULL default '0',
  `reporteeProfileId` int(10) unsigned NOT NULL default '0',
  `complaint` varchar(255) NOT NULL default '',
  `kickNow` tinyint(3) unsigned NOT NULL default '0',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`abuseId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Log_Abuse_Comments`
--

DROP TABLE IF EXISTS `Log_Abuse_Comments`;
CREATE TABLE IF NOT EXISTS `Log_Abuse_Comments` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `abuseId` int(10) unsigned NOT NULL default '0',
  `comment` text NOT NULL,
  `status` enum('open','closed') NOT NULL default 'open',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`),
  KEY `abuseId` (`abuseId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Log_Ban`
--

DROP TABLE IF EXISTS `Log_Ban`;
CREATE TABLE IF NOT EXISTS `Log_Ban` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `type` enum('not_banned','cdkey','account') NOT NULL default 'not_banned',
  `numSeconds` int(11) NOT NULL default '0',
  `banLifts` datetime NOT NULL default '0000-00-00 00:00:00',
  `emailOrCdKey` varchar(255) NOT NULL default '',
  `reason` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `emailOrCdKey` (`emailOrCdKey`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Log_ChangeEmail`
--

DROP TABLE IF EXISTS `Log_ChangeEmail`;
CREATE TABLE IF NOT EXISTS `Log_ChangeEmail` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `oldEmail` varchar(128) NOT NULL default '',
  `newEmail` varchar(128) NOT NULL default '',
  `accountId` int(11) NOT NULL default '0',
  `timeChanged` timestamp NOT NULL default CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Log_ChatMessages`
--

DROP TABLE IF EXISTS `Log_ChatMessages`;
CREATE TABLE IF NOT EXISTS `Log_ChatMessages` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `userProfileId` int(10) unsigned NOT NULL default '0',
  `chatRoomId` int(10) unsigned NOT NULL default '0',
  `message` varchar(255) NOT NULL default '',
  `submissionTime` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Log_ChatRoomCreation`
--

DROP TABLE IF EXISTS `Log_ChatRoomCreation`;
CREATE TABLE IF NOT EXISTS `Log_ChatRoomCreation` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `chatRoomId` int(10) unsigned NOT NULL default '0',
  `name` varchar(255) NOT NULL default '',
  `creationTime` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `createdByProfileId` int(10) unsigned NOT NULL default '0',
  `password` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Log_DetailedOnline`
--

DROP TABLE IF EXISTS `Log_DetailedOnline`;
CREATE TABLE IF NOT EXISTS `Log_DetailedOnline` (
  `sampleTime` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `numOnline` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`sampleTime`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Log_login`
--

DROP TABLE IF EXISTS `Log_login`;
CREATE TABLE IF NOT EXISTS `Log_login` (
  `login_id` int(11) unsigned NOT NULL auto_increment,
  `logintime` datetime NOT NULL default '0000-00-00 00:00:00',
  `accountid` int(11) unsigned NOT NULL default '0',
  `productid` int(11) unsigned NOT NULL default '0',
  `loginstatus` int(11) unsigned NOT NULL default '0',
  `fromIP` varchar(16) character set latin1 NOT NULL default '0',
  `CdKeySequence` int(11) unsigned NOT NULL default '0',
  `fingerprint` int(11) unsigned NOT NULL default '0',
  `sessionTime` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`login_id`),
  KEY `accountid` (`accountid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Log_OnlineByContinent`
--

DROP TABLE IF EXISTS `Log_OnlineByContinent`;
CREATE TABLE IF NOT EXISTS `Log_OnlineByContinent` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `continent` varchar(5) NOT NULL default '--',
  `playerCount` int(10) unsigned NOT NULL default '0',
  `aTimestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Log_Recruit`
--

DROP TABLE IF EXISTS `Log_Recruit`;
CREATE TABLE IF NOT EXISTS `Log_Recruit` (
  `recruiteeAccountId` int(10) unsigned NOT NULL default '0',
  `recruiterAccountId` int(10) unsigned NOT NULL default '0',
  `aTimestamp` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  UNIQUE KEY `recruiteeAccountId` (`recruiteeAccountId`),
  KEY `recruiterAccountId` (`recruiterAccountId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `MapHashes`
--

DROP TABLE IF EXISTS `MapHashes`;
CREATE TABLE IF NOT EXISTS `MapHashes` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `hash` bigint(20) unsigned NOT NULL default '0',
  `name` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `hash` (`hash`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MatchStats`
--

DROP TABLE IF EXISTS `MatchStats`;
CREATE TABLE IF NOT EXISTS `MatchStats` (
  `matchId` int(10) unsigned NOT NULL auto_increment,
  `mapHash` bigint(20) unsigned NOT NULL default '0',
  `winFaction` enum('NONE','USA','NATO','USSR','CLAN') NOT NULL default 'NONE',
  `matchTime` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `matchLength` int(10) unsigned NOT NULL default '0',
  `numPlayersAtEnd` tinyint(3) unsigned NOT NULL default '0',
  `cycleHash` bigint(20) unsigned NOT NULL default '0',
  `serverId` int(10) unsigned NOT NULL default '0',
  `averageExperience` int(10) unsigned NOT NULL default '0',
  `serverType` enum('normal','clan') NOT NULL default 'normal',
  PRIMARY KEY  (`matchId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MatchStatsPerPlayer`
--

DROP TABLE IF EXISTS `MatchStatsPerPlayer`;
CREATE TABLE IF NOT EXISTS `MatchStatsPerPlayer` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `playedAt` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `profileId` int(10) unsigned NOT NULL default '0',
  `maphash` bigint(20) unsigned NOT NULL default '0',
  `scoreTotal` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MatchStatsPerRole`
--

DROP TABLE IF EXISTS `MatchStatsPerRole`;
CREATE TABLE IF NOT EXISTS `MatchStatsPerRole` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `mapHash` bigint(20) unsigned NOT NULL default '0',
  `playedAt` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `role` int(10) unsigned NOT NULL default '0',
  `numWin` int(10) unsigned NOT NULL default '0',
  `numLoss` int(10) unsigned NOT NULL default '0',
  `scoreWin` int(10) unsigned NOT NULL default '0',
  `scoreLoss` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MA_comments`
--

DROP TABLE IF EXISTS `MA_comments`;
CREATE TABLE IF NOT EXISTS `MA_comments` (
  `commentId` int(10) unsigned NOT NULL auto_increment,
  `author` varchar(80) NOT NULL default '',
  `idType` enum('user_web','user_massgate','ticket') default NULL,
  `refId` int(10) unsigned NOT NULL default '0',
  `comment` text,
  `datePosted` timestamp NOT NULL default CURRENT_TIMESTAMP,
  PRIMARY KEY  (`commentId`),
  KEY `reference` (`idType`,`refId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MA_tickets`
--

DROP TABLE IF EXISTS `MA_tickets`;
CREATE TABLE IF NOT EXISTS `MA_tickets` (
  `ticketId` int(10) unsigned NOT NULL auto_increment,
  `source` enum('web','massgate','unspecified') NOT NULL default 'unspecified',
  `owner` varchar(80) NOT NULL default 'nobody',
  `status` enum('open','closed','attn') NOT NULL default 'open',
  `action` enum('none','warning','punishment') NOT NULL default 'none',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `timeAdded` datetime default NULL,
  PRIMARY KEY  (`ticketId`),
  KEY `owner` (`owner`),
  KEY `status` (`status`),
  KEY `source` (`source`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MA_ticket_changes`
--

DROP TABLE IF EXISTS `MA_ticket_changes`;
CREATE TABLE IF NOT EXISTS `MA_ticket_changes` (
  `changeId` int(10) unsigned NOT NULL auto_increment,
  `commentId` int(10) unsigned NOT NULL default '0',
  `field` varchar(100) NOT NULL default '',
  `oldvalue` text,
  `newvalue` text,
  PRIMARY KEY  (`changeId`),
  KEY `commentId` (`commentId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MA_ticket_massgate_xref`
--

DROP TABLE IF EXISTS `MA_ticket_massgate_xref`;
CREATE TABLE IF NOT EXISTS `MA_ticket_massgate_xref` (
  `ticketId` int(10) unsigned NOT NULL default '0',
  `abuseId` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`ticketId`,`abuseId`),
  KEY `abuseId` (`abuseId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MA_ticket_web_xref`
--

DROP TABLE IF EXISTS `MA_ticket_web_xref`;
CREATE TABLE IF NOT EXISTS `MA_ticket_web_xref` (
  `ticketId` int(10) unsigned NOT NULL default '0',
  `reporter_user_id` int(10) unsigned NOT NULL default '0',
  `reportee_user_id` int(10) unsigned NOT NULL default '0',
  `complaint` text,
  PRIMARY KEY  (`ticketId`,`reporter_user_id`,`reportee_user_id`),
  KEY `reporter_user_id` (`reporter_user_id`),
  KEY `reportee_user_id` (`reportee_user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_banlists`
--

DROP TABLE IF EXISTS `MW_banlists`;
CREATE TABLE IF NOT EXISTS `MW_banlists` (
  `id` int(11) NOT NULL auto_increment,
  `forum_id` int(11) NOT NULL default '0',
  `type` tinyint(4) NOT NULL default '0',
  `pcre` tinyint(4) NOT NULL default '0',
  `string` varchar(255) NOT NULL default '',
  `comments` text NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `forum_id` (`forum_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_DontMailList`
--

DROP TABLE IF EXISTS `MW_DontMailList`;
CREATE TABLE IF NOT EXISTS `MW_DontMailList` (
  `email` varchar(150) NOT NULL default ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_event_logging`
--

DROP TABLE IF EXISTS `MW_event_logging`;
CREATE TABLE IF NOT EXISTS `MW_event_logging` (
  `log_id` int(10) unsigned NOT NULL auto_increment,
  `source` varchar(32) NOT NULL default 'unknown',
  `category` tinyint(4) NOT NULL default '0',
  `loglevel` tinyint(4) NOT NULL default '0',
  `message` varchar(255) NOT NULL default '<no error message specified>',
  `details` text,
  `ip` varchar(15) default NULL,
  `hostname` varchar(255) default NULL,
  `user_id` int(10) unsigned default NULL,
  `datestamp` int(10) unsigned NOT NULL default '0',
  `vroot` int(10) unsigned default NULL,
  `forum_id` int(10) unsigned default NULL,
  `thread_id` int(10) unsigned default NULL,
  `message_id` int(10) unsigned default NULL,
  PRIMARY KEY  (`log_id`),
  KEY `source` (`source`),
  KEY `category` (`category`),
  KEY `loglevel` (`loglevel`),
  KEY `datestamp` (`datestamp`),
  KEY `user_id` (`user_id`),
  KEY `forum` (`vroot`,`forum_id`,`thread_id`,`message_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `MW_files`
--

DROP TABLE IF EXISTS `MW_files`;
CREATE TABLE IF NOT EXISTS `MW_files` (
  `file_id` int(11) NOT NULL auto_increment,
  `user_id` int(11) NOT NULL default '0',
  `filename` varchar(255) NOT NULL default '',
  `filesize` int(11) NOT NULL default '0',
  `file_data` mediumtext NOT NULL,
  `add_datetime` int(10) unsigned NOT NULL default '0',
  `message_id` int(10) unsigned NOT NULL default '0',
  `link` varchar(10) NOT NULL default '',
  PRIMARY KEY  (`file_id`),
  KEY `add_datetime` (`add_datetime`),
  KEY `message_id_link` (`message_id`,`link`),
  KEY `user_id_link` (`user_id`,`link`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_forums`
--

DROP TABLE IF EXISTS `MW_forums`;
CREATE TABLE IF NOT EXISTS `MW_forums` (
  `forum_id` int(10) unsigned NOT NULL auto_increment,
  `name` varchar(50) NOT NULL default '',
  `active` smallint(6) NOT NULL default '0',
  `description` text,
  `template` varchar(50) NOT NULL default '',
  `folder_flag` tinyint(1) NOT NULL default '0',
  `parent_id` int(10) unsigned NOT NULL default '0',
  `list_length_flat` int(10) unsigned NOT NULL default '0',
  `list_length_threaded` int(10) unsigned NOT NULL default '0',
  `moderation` int(10) unsigned NOT NULL default '0',
  `threaded_list` tinyint(4) NOT NULL default '0',
  `threaded_read` tinyint(4) NOT NULL default '0',
  `float_to_top` tinyint(4) NOT NULL default '0',
  `check_duplicate` tinyint(4) NOT NULL default '0',
  `allow_attachment_types` varchar(100) NOT NULL default '',
  `max_attachment_size` int(10) unsigned NOT NULL default '0',
  `max_totalattachment_size` int(10) unsigned NOT NULL default '0',
  `max_attachments` int(10) unsigned NOT NULL default '0',
  `pub_perms` int(10) unsigned NOT NULL default '0',
  `reg_perms` int(10) unsigned NOT NULL default '0',
  `display_ip_address` smallint(5) unsigned NOT NULL default '1',
  `allow_email_notify` smallint(5) unsigned NOT NULL default '1',
  `language` varchar(100) NOT NULL default 'english',
  `email_moderators` tinyint(1) NOT NULL default '0',
  `message_count` int(10) unsigned NOT NULL default '0',
  `sticky_count` int(10) unsigned NOT NULL default '0',
  `thread_count` int(10) unsigned NOT NULL default '0',
  `last_post_time` int(10) unsigned NOT NULL default '0',
  `display_order` int(10) unsigned NOT NULL default '0',
  `read_length` int(10) unsigned NOT NULL default '0',
  `vroot` int(10) unsigned NOT NULL default '0',
  `edit_post` tinyint(1) NOT NULL default '1',
  `template_settings` text,
  `count_views` tinyint(1) unsigned NOT NULL default '0',
  `display_fixed` tinyint(1) unsigned NOT NULL default '0',
  `reverse_threading` tinyint(1) NOT NULL default '0',
  `inherit_id` int(10) unsigned default NULL,
  `cache_version` int(10) unsigned NOT NULL default '0',
  `forum_path` text NOT NULL,
  `count_views_per_thread` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`forum_id`),
  KEY `name` (`name`),
  KEY `active` (`active`,`parent_id`),
  KEY `group_id` (`parent_id`),
  KEY `vroot` (`vroot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_forum_group_xref`
--

DROP TABLE IF EXISTS `MW_forum_group_xref`;
CREATE TABLE IF NOT EXISTS `MW_forum_group_xref` (
  `forum_id` int(11) NOT NULL default '0',
  `group_id` int(11) NOT NULL default '0',
  `permission` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`forum_id`,`group_id`),
  KEY `group_id` (`group_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_groups`
--

DROP TABLE IF EXISTS `MW_groups`;
CREATE TABLE IF NOT EXISTS `MW_groups` (
  `group_id` int(11) NOT NULL auto_increment,
  `name` varchar(255) NOT NULL default '0',
  `open` tinyint(3) NOT NULL default '0',
  `clanId` int(10) unsigned default NULL,
  PRIMARY KEY  (`group_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_massgate_export`
--

DROP TABLE IF EXISTS `MW_massgate_export`;
CREATE TABLE IF NOT EXISTS `MW_massgate_export` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `export_type` enum('new','update','delete') NOT NULL default 'new',
  `data_type` enum('profile','clan','account') NOT NULL default 'profile',
  `num1` int(10) unsigned default NULL,
  `num2` int(10) unsigned default NULL,
  `num3` int(10) unsigned default NULL,
  `text1` text,
  `text2` text,
  `text3` text,
  `imported` tinyint(1) unsigned NOT NULL default '0',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_messages`
--

DROP TABLE IF EXISTS `MW_messages`;
CREATE TABLE IF NOT EXISTS `MW_messages` (
  `message_id` int(10) unsigned NOT NULL auto_increment,
  `forum_id` int(10) unsigned NOT NULL default '0',
  `thread` int(10) unsigned NOT NULL default '0',
  `parent_id` int(10) unsigned NOT NULL default '0',
  `author` varchar(255) NOT NULL default '',
  `subject` varchar(255) NOT NULL default '',
  `body` text NOT NULL,
  `email` varchar(100) NOT NULL default '',
  `ip` varchar(255) NOT NULL default '',
  `status` tinyint(4) NOT NULL default '2',
  `msgid` varchar(100) NOT NULL default '',
  `modifystamp` int(10) unsigned NOT NULL default '0',
  `user_id` int(10) unsigned NOT NULL default '0',
  `thread_count` int(10) unsigned NOT NULL default '0',
  `moderator_post` tinyint(3) unsigned NOT NULL default '0',
  `sort` tinyint(4) NOT NULL default '2',
  `datestamp` int(10) unsigned NOT NULL default '0',
  `meta` mediumtext,
  `viewcount` int(10) unsigned NOT NULL default '0',
  `closed` tinyint(4) NOT NULL default '0',
  `recent_message_id` int(10) unsigned NOT NULL default '0',
  `recent_user_id` int(10) unsigned NOT NULL default '0',
  `recent_author` varchar(255) NOT NULL default '',
  `moved` tinyint(1) NOT NULL default '0',
  `threadviewcount` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`message_id`),
  KEY `thread_message` (`thread`,`message_id`),
  KEY `thread_forum` (`thread`,`forum_id`),
  KEY `special_threads` (`sort`,`forum_id`),
  KEY `status_forum` (`status`,`forum_id`),
  KEY `list_page_float` (`forum_id`,`parent_id`,`modifystamp`),
  KEY `list_page_flat` (`forum_id`,`parent_id`,`thread`),
  KEY `forum_max_message` (`forum_id`,`message_id`,`status`,`parent_id`),
  KEY `last_post_time` (`forum_id`,`status`,`modifystamp`),
  KEY `next_prev_thread` (`forum_id`,`status`,`thread`),
  KEY `dup_check` (`forum_id`,`author`(50),`subject`,`datestamp`),
  KEY `recent_user_id` (`recent_user_id`),
  KEY `new_count` (`forum_id`,`status`,`moved`,`message_id`),
  KEY `new_threads` (`forum_id`,`status`,`parent_id`,`moved`,`message_id`),
  KEY `updated_threads` (`status`,`parent_id`,`modifystamp`),
  KEY `user_messages` (`user_id`,`message_id`),
  KEY `recent_threads` (`status`,`parent_id`,`message_id`,`forum_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_messages_edittrack`
--

DROP TABLE IF EXISTS `MW_messages_edittrack`;
CREATE TABLE IF NOT EXISTS `MW_messages_edittrack` (
  `track_id` int(10) unsigned NOT NULL auto_increment,
  `message_id` int(10) unsigned NOT NULL default '0',
  `user_id` int(10) unsigned NOT NULL default '0',
  `time` int(10) unsigned NOT NULL default '0',
  `diff_body` text,
  `diff_subject` text,
  PRIMARY KEY  (`track_id`),
  KEY `message_id` (`message_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_open_beta_invite_log`
--

DROP TABLE IF EXISTS `MW_open_beta_invite_log`;
CREATE TABLE IF NOT EXISTS `MW_open_beta_invite_log` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `sequenceNumber` int(10) unsigned NOT NULL default '0',
  `accessCode` int(10) unsigned default NULL,
  `email` varchar(150) NOT NULL default '',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `sequenceNumber` (`sequenceNumber`),
  UNIQUE KEY `accessCode` (`accessCode`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_password_recovery`
--

DROP TABLE IF EXISTS `MW_password_recovery`;
CREATE TABLE IF NOT EXISTS `MW_password_recovery` (
  `recovery_id` int(10) unsigned NOT NULL auto_increment,
  `email` varchar(100) NOT NULL default '',
  `security_string` varchar(100) NOT NULL default '',
  `ip` varchar(20) NOT NULL default '',
  `created_at` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `used` int(1) NOT NULL default '0',
  PRIMARY KEY  (`recovery_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_pm_buddies`
--

DROP TABLE IF EXISTS `MW_pm_buddies`;
CREATE TABLE IF NOT EXISTS `MW_pm_buddies` (
  `pm_buddy_id` int(10) unsigned NOT NULL auto_increment,
  `user_id` int(10) unsigned NOT NULL default '0',
  `buddy_user_id` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pm_buddy_id`),
  UNIQUE KEY `userids` (`user_id`,`buddy_user_id`),
  KEY `buddy_user_id` (`buddy_user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_pm_folders`
--

DROP TABLE IF EXISTS `MW_pm_folders`;
CREATE TABLE IF NOT EXISTS `MW_pm_folders` (
  `pm_folder_id` int(10) unsigned NOT NULL auto_increment,
  `user_id` int(10) unsigned NOT NULL default '0',
  `foldername` varchar(20) NOT NULL default '',
  PRIMARY KEY  (`pm_folder_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_pm_messages`
--

DROP TABLE IF EXISTS `MW_pm_messages`;
CREATE TABLE IF NOT EXISTS `MW_pm_messages` (
  `pm_message_id` int(10) unsigned NOT NULL auto_increment,
  `user_id` int(10) unsigned NOT NULL default '0',
  `author` varchar(255) NOT NULL default '',
  `subject` varchar(100) NOT NULL default '',
  `message` text NOT NULL,
  `datestamp` int(10) unsigned NOT NULL default '0',
  `meta` mediumtext,
  PRIMARY KEY  (`pm_message_id`),
  KEY `user_id` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_pm_xref`
--

DROP TABLE IF EXISTS `MW_pm_xref`;
CREATE TABLE IF NOT EXISTS `MW_pm_xref` (
  `pm_xref_id` int(10) unsigned NOT NULL auto_increment,
  `user_id` int(10) unsigned NOT NULL default '0',
  `pm_folder_id` int(10) unsigned NOT NULL default '0',
  `special_folder` varchar(10) default NULL,
  `pm_message_id` int(10) unsigned NOT NULL default '0',
  `read_flag` tinyint(1) NOT NULL default '0',
  `reply_flag` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`pm_xref_id`),
  KEY `xref` (`user_id`,`pm_folder_id`,`pm_message_id`),
  KEY `read_flag` (`read_flag`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_profile_user_xref`
--

DROP TABLE IF EXISTS `MW_profile_user_xref`;
CREATE TABLE IF NOT EXISTS `MW_profile_user_xref` (
  `profileId` int(10) unsigned NOT NULL default '0',
  `user_id` int(10) unsigned NOT NULL default '0',
  UNIQUE KEY `profileId` (`profileId`,`user_id`),
  KEY `user_id` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_search`
--

DROP TABLE IF EXISTS `MW_search`;
CREATE TABLE IF NOT EXISTS `MW_search` (
  `message_id` int(10) unsigned NOT NULL default '0',
  `forum_id` int(10) unsigned NOT NULL default '0',
  `search_text` mediumtext NOT NULL,
  PRIMARY KEY  (`message_id`),
  KEY `forum_id` (`forum_id`),
  FULLTEXT KEY `search_text` (`search_text`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_settings`
--

DROP TABLE IF EXISTS `MW_settings`;
CREATE TABLE IF NOT EXISTS `MW_settings` (
  `name` varchar(255) NOT NULL default '',
  `type` enum('V','S') NOT NULL default 'V',
  `data` text NOT NULL,
  PRIMARY KEY  (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_spamhurdles`
--

DROP TABLE IF EXISTS `MW_spamhurdles`;
CREATE TABLE IF NOT EXISTS `MW_spamhurdles` (
  `id` varchar(32) NOT NULL default '',
  `data` mediumtext NOT NULL,
  `create_time` int(10) unsigned NOT NULL default '0',
  `expire_time` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `MW_ssotd`
--

DROP TABLE IF EXISTS `MW_ssotd`;
CREATE TABLE IF NOT EXISTS `MW_ssotd` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `filename` varchar(255) NOT NULL default '',
  `submitter` varchar(255) NOT NULL default '',
  `showdate` varchar(10) default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `showdate` (`showdate`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_store_cdKeys`
--

DROP TABLE IF EXISTS `MW_store_cdKeys`;
CREATE TABLE IF NOT EXISTS `MW_store_cdKeys` (
  `seqNr` int(10) unsigned NOT NULL auto_increment,
  `cdKey` varchar(200) NOT NULL default '',
  `type` enum('permanent','limited') default NULL,
  PRIMARY KEY  (`seqNr`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_store_Key_Transaction_xref`
--

DROP TABLE IF EXISTS `MW_store_Key_Transaction_xref`;
CREATE TABLE IF NOT EXISTS `MW_store_Key_Transaction_xref` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `seqNr` int(10) unsigned NOT NULL default '0',
  `txn_id` varchar(25) NOT NULL default '',
  PRIMARY KEY  (`id`),
  KEY `seqNr` (`seqNr`,`txn_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_store_Paypal_IPN_log`
--

DROP TABLE IF EXISTS `MW_store_Paypal_IPN_log`;
CREATE TABLE IF NOT EXISTS `MW_store_Paypal_IPN_log` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `txn_id` varchar(25) NOT NULL default '',
  `ipn_status` enum('verified','invalid') default NULL,
  `payer_email` varchar(250) NOT NULL default '',
  `payer_id` varchar(25) NOT NULL default '',
  `receiver_email` varchar(250) NOT NULL default '',
  `receiver_id` varchar(25) NOT NULL default '',
  `payment_status` varchar(50) NOT NULL default '',
  `payment_type` varchar(75) default NULL,
  `item_name` varchar(250) NOT NULL default '',
  `payment_gross` varchar(10) NOT NULL default '0',
  `currency` varchar(5) NOT NULL default 'USD',
  `txn_type` varchar(50) default NULL,
  `payment_date` varchar(35) NOT NULL default '',
  `last_name` varchar(250) NOT NULL default '',
  `residence_country` varchar(5) NOT NULL default '',
  `payer_status` varchar(25) NOT NULL default '',
  `dateAdded` datetime NOT NULL default '0000-00-00 00:00:00',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`),
  KEY `txn_id` (`txn_id`,`payer_email`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_subscribers`
--

DROP TABLE IF EXISTS `MW_subscribers`;
CREATE TABLE IF NOT EXISTS `MW_subscribers` (
  `user_id` int(10) unsigned NOT NULL default '0',
  `forum_id` int(10) unsigned NOT NULL default '0',
  `sub_type` tinyint(4) NOT NULL default '0',
  `thread` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`user_id`,`forum_id`,`thread`),
  KEY `forum_id` (`forum_id`,`thread`,`sub_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_usage_tracking`
--

DROP TABLE IF EXISTS `MW_usage_tracking`;
CREATE TABLE IF NOT EXISTS `MW_usage_tracking` (
  `k` varchar(50) NOT NULL default '',
  `total_counter` int(10) unsigned NOT NULL default '0',
  `logged_in_counter` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`k`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_users`
--

DROP TABLE IF EXISTS `MW_users`;
CREATE TABLE IF NOT EXISTS `MW_users` (
  `user_id` int(10) unsigned NOT NULL auto_increment,
  `username` varchar(50) NOT NULL default '',
  `password` varchar(50) NOT NULL default '',
  `sessid_lt` varchar(50) NOT NULL default '',
  `sessid_st` varchar(50) NOT NULL default '',
  `sessid_st_timeout` int(10) unsigned NOT NULL default '0',
  `password_temp` varchar(50) NOT NULL default '',
  `email` varchar(100) NOT NULL default '',
  `email_temp` varchar(110) NOT NULL default '',
  `hide_email` tinyint(1) NOT NULL default '1',
  `active` tinyint(1) NOT NULL default '0',
  `signature` text,
  `threaded_list` tinyint(4) NOT NULL default '0',
  `posts` int(10) NOT NULL default '0',
  `admin` tinyint(1) NOT NULL default '0',
  `threaded_read` tinyint(4) NOT NULL default '0',
  `date_added` int(10) unsigned NOT NULL default '0',
  `date_last_active` int(10) unsigned NOT NULL default '0',
  `last_active_forum` int(10) unsigned NOT NULL default '0',
  `hide_activity` tinyint(1) NOT NULL default '0',
  `show_signature` tinyint(1) NOT NULL default '0',
  `email_notify` tinyint(1) NOT NULL default '0',
  `pm_email_notify` tinyint(1) NOT NULL default '0',
  `tz_offset` float(4,2) NOT NULL default '-99.00',
  `is_dst` tinyint(1) NOT NULL default '0',
  `user_language` varchar(100) NOT NULL default '',
  `user_template` varchar(100) NOT NULL default '',
  `moderator_data` text,
  `moderation_email` tinyint(2) unsigned NOT NULL default '1',
  `settings_data` mediumtext NOT NULL,
  `real_name` varchar(255) NOT NULL default '',
  `display_name` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`user_id`),
  UNIQUE KEY `username` (`username`),
  KEY `active` (`active`),
  KEY `userpass` (`username`,`password`),
  KEY `sessid_st` (`sessid_st`),
  KEY `cookie_sessid_lt` (`sessid_lt`),
  KEY `activity` (`date_last_active`,`hide_activity`,`last_active_forum`),
  KEY `date_added` (`date_added`),
  KEY `email` (`email`),
  KEY `real_name` (`real_name`),
  KEY `admin` (`admin`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_user_custom_fields`
--

DROP TABLE IF EXISTS `MW_user_custom_fields`;
CREATE TABLE IF NOT EXISTS `MW_user_custom_fields` (
  `user_id` int(11) NOT NULL default '0',
  `type` int(11) NOT NULL default '0',
  `data` text,
  PRIMARY KEY  (`user_id`,`type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_user_group_xref`
--

DROP TABLE IF EXISTS `MW_user_group_xref`;
CREATE TABLE IF NOT EXISTS `MW_user_group_xref` (
  `user_id` int(11) NOT NULL default '0',
  `group_id` int(11) NOT NULL default '0',
  `status` tinyint(3) NOT NULL default '1',
  PRIMARY KEY  (`user_id`,`group_id`),
  KEY `group_id` (`group_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_user_newflags`
--

DROP TABLE IF EXISTS `MW_user_newflags`;
CREATE TABLE IF NOT EXISTS `MW_user_newflags` (
  `user_id` int(11) NOT NULL default '0',
  `forum_id` int(11) NOT NULL default '0',
  `message_id` int(11) NOT NULL default '0',
  PRIMARY KEY  (`user_id`,`forum_id`,`message_id`),
  KEY `message_id` (`message_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `MW_user_permissions`
--

DROP TABLE IF EXISTS `MW_user_permissions`;
CREATE TABLE IF NOT EXISTS `MW_user_permissions` (
  `user_id` int(10) unsigned NOT NULL default '0',
  `forum_id` int(10) unsigned NOT NULL default '0',
  `permission` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`user_id`,`forum_id`),
  KEY `forum_id` (`forum_id`,`permission`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `OptionalContent`
--

DROP TABLE IF EXISTS `OptionalContent`;
CREATE TABLE IF NOT EXISTS `OptionalContent` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `hash` int(10) unsigned NOT NULL default '0',
  `name` varchar(255) NOT NULL default '',
  `url` varchar(255) NOT NULL default '',
  `groupMembership` int(10) unsigned NOT NULL default '0',
  `country` varchar(2) NOT NULL default '',
  `continent` varchar(2) NOT NULL default '',
  `lang` varchar(2) NOT NULL default '',
  `n_retries` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `PCC`
--

DROP TABLE IF EXISTS `PCC`;
CREATE TABLE IF NOT EXISTS `PCC` (
  `id` int(11) NOT NULL auto_increment,
  `pccId` int(11) default NULL,
  `type` int(3) default NULL,
  `url` varchar(255) default NULL,
  `status` enum('Public','Banned') default NULL,
  `seqNum` int(3) default NULL,
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`),
  KEY `status` (`status`),
  KEY `pccId` (`pccId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `PlayerBadges`
--

DROP TABLE IF EXISTS `PlayerBadges`;
CREATE TABLE IF NOT EXISTS `PlayerBadges` (
  `profileId` int(10) unsigned NOT NULL default '0',
  `infSp` tinyint(1) unsigned NOT NULL default '0',
  `infSpStars` tinyint(1) unsigned NOT NULL default '0',
  `airSp` tinyint(1) unsigned NOT NULL default '0',
  `airSpStars` tinyint(1) unsigned NOT NULL default '0',
  `armSp` tinyint(1) unsigned NOT NULL default '0',
  `armSpStars` tinyint(1) unsigned NOT NULL default '0',
  `supSp` tinyint(1) unsigned NOT NULL default '0',
  `supSpStars` tinyint(1) unsigned NOT NULL default '0',
  `scoreAch` tinyint(1) unsigned NOT NULL default '0',
  `scoreAchStars` tinyint(1) unsigned NOT NULL default '0',
  `cpAch` tinyint(1) unsigned NOT NULL default '0',
  `cpAchStars` tinyint(1) unsigned NOT NULL default '0',
  `fortAch` tinyint(1) unsigned NOT NULL default '0',
  `fortAchStars` tinyint(1) unsigned NOT NULL default '0',
  `mgAch` tinyint(1) unsigned NOT NULL default '0',
  `mgAchStars` tinyint(1) unsigned NOT NULL default '0',
  `matchAch` tinyint(1) unsigned NOT NULL default '0',
  `matchAchStars` tinyint(1) unsigned NOT NULL default '0',
  `USAAch` tinyint(1) unsigned NOT NULL default '0',
  `USAAchStars` tinyint(1) unsigned NOT NULL default '0',
  `USSRAch` tinyint(1) unsigned NOT NULL default '0',
  `USSRAchStars` tinyint(1) unsigned NOT NULL default '0',
  `NATOAch` tinyint(1) unsigned NOT NULL default '0',
  `NATOAchStars` tinyint(1) unsigned NOT NULL default '0',
  `preOrdAch` tinyint(1) unsigned NOT NULL default '0',
  `preOrdAchStars` tinyint(1) unsigned NOT NULL default '0',
  `reqruitAFriendAch` tinyint(1) unsigned NOT NULL default '0',
  `reqruitAFriendAchStars` tinyint(1) unsigned NOT NULL default '0',
  `lastUpdated` timestamp NOT NULL default '0000-00-00 00:00:00' on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`profileId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `PlayerLadder`
--

DROP TABLE IF EXISTS `PlayerLadder`;
CREATE TABLE IF NOT EXISTS `PlayerLadder` (
  `id` int(10) unsigned NOT NULL default '0',
  `scoreOnDay` int(11) NOT NULL default '0',
  `dayOfEntry` date NOT NULL default '0000-00-00',
  UNIQUE KEY `id` (`id`,`dayOfEntry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `PlayerMedals`
--

DROP TABLE IF EXISTS `PlayerMedals`;
CREATE TABLE IF NOT EXISTS `PlayerMedals` (
  `profileId` int(10) unsigned NOT NULL default '0',
  `infAch` tinyint(1) unsigned NOT NULL default '0',
  `infAchStars` tinyint(1) unsigned NOT NULL default '0',
  `airAch` tinyint(1) unsigned NOT NULL default '0',
  `airAchStars` tinyint(1) unsigned NOT NULL default '0',
  `armorAch` tinyint(1) unsigned NOT NULL default '0',
  `armorAchStars` tinyint(1) unsigned NOT NULL default '0',
  `supAch` tinyint(1) unsigned NOT NULL default '0',
  `supAchStars` tinyint(1) unsigned NOT NULL default '0',
  `scoreAch` tinyint(1) unsigned NOT NULL default '0',
  `scoreAchStars` tinyint(1) unsigned NOT NULL default '0',
  `taAch` tinyint(1) unsigned NOT NULL default '0',
  `taAchStars` tinyint(1) unsigned NOT NULL default '0',
  `infComEx` tinyint(1) unsigned NOT NULL default '0',
  `infComExStars` tinyint(1) unsigned NOT NULL default '0',
  `airComEx` tinyint(1) unsigned NOT NULL default '0',
  `airComExStars` tinyint(1) unsigned NOT NULL default '0',
  `armorComEx` tinyint(1) unsigned NOT NULL default '0',
  `armorComExStars` tinyint(1) unsigned NOT NULL default '0',
  `supComEx` tinyint(1) unsigned NOT NULL default '0',
  `supComExStars` tinyint(1) unsigned NOT NULL default '0',
  `winStreak` tinyint(1) unsigned NOT NULL default '0',
  `winStreakStars` tinyint(1) unsigned NOT NULL default '0',
  `domSpec` tinyint(1) unsigned NOT NULL default '0',
  `domSpecStars` tinyint(1) unsigned NOT NULL default '0',
  `domEx` tinyint(1) unsigned NOT NULL default '0',
  `domExStars` tinyint(1) unsigned NOT NULL default '0',
  `assSpec` tinyint(1) unsigned NOT NULL default '0',
  `assSpecStars` tinyint(1) unsigned NOT NULL default '0',
  `assEx` tinyint(1) unsigned NOT NULL default '0',
  `assExStars` tinyint(1) unsigned NOT NULL default '0',
  `towSpec` tinyint(1) unsigned NOT NULL default '0',
  `towSpecStars` tinyint(1) unsigned NOT NULL default '0',
  `towEx` tinyint(1) unsigned NOT NULL default '0',
  `towExStars` tinyint(1) unsigned NOT NULL default '0',
  `nukeSpec` tinyint(1) unsigned NOT NULL default '0',
  `nukeSpecStars` tinyint(1) unsigned NOT NULL default '0',
  `highDec` tinyint(1) unsigned NOT NULL default '0',
  `highDecStars` tinyint(1) unsigned NOT NULL default '0',
  `lastUpdated` timestamp NOT NULL default '0000-00-00 00:00:00' on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`profileId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `PlayerStats`
--

DROP TABLE IF EXISTS `PlayerStats`;
CREATE TABLE IF NOT EXISTS `PlayerStats` (
  `profileId` int(10) unsigned NOT NULL default '0',
  `rank` tinyint(3) unsigned NOT NULL default '0',
  `maxLadderPercent` tinyint(3) unsigned NOT NULL default '0',
  `massgateMemberSince` timestamp NOT NULL default '0000-00-00 00:00:00',
  `disallowStatsUntil` datetime NOT NULL default '0000-00-00 00:00:00',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `sc_tot` int(11) NOT NULL default '0',
  `sc_highest` int(11) NOT NULL default '0',
  `sc_inf` int(11) NOT NULL default '0',
  `sc_highinf` int(11) NOT NULL default '0',
  `sc_sup` int(11) NOT NULL default '0',
  `sc_highsup` int(11) NOT NULL default '0',
  `sc_arm` int(11) NOT NULL default '0',
  `sc_higharm` int(11) NOT NULL default '0',
  `sc_air` int(11) NOT NULL default '0',
  `sc_highair` int(11) NOT NULL default '0',
  `sc_damen` int(11) NOT NULL default '0',
  `sc_ta` int(11) NOT NULL default '0',
  `sc_cpc` int(11) NOT NULL default '0',
  `sc_rep` int(11) NOT NULL default '0',
  `sc_fort` int(11) NOT NULL default '0',
  `sc_trans` int(11) NOT NULL default '0',
  `sc_tk` int(11) NOT NULL default '0',
  `t_USA` int(10) unsigned NOT NULL default '0',
  `t_USSR` int(10) unsigned NOT NULL default '0',
  `t_NATO` int(10) unsigned NOT NULL default '0',
  `t_sup` int(10) unsigned NOT NULL default '0',
  `t_arm` int(10) unsigned NOT NULL default '0',
  `t_air` int(10) unsigned NOT NULL default '0',
  `t_inf` int(10) unsigned NOT NULL default '0',
  `n_matches` int(10) unsigned NOT NULL default '0',
  `n_matcheswon` int(10) unsigned NOT NULL default '0',
  `n_matcheslost` int(10) unsigned NOT NULL default '0',
  `n_towmatches` int(10) unsigned NOT NULL default '0',
  `n_towmatcheswon` int(10) unsigned NOT NULL default '0',
  `n_towstreak` int(10) unsigned NOT NULL default '0',
  `n_assmatches` int(10) unsigned NOT NULL default '0',
  `n_assmatcheswon` int(10) unsigned NOT NULL default '0',
  `n_assstreak` int(10) unsigned NOT NULL default '0',
  `n_dommatches` int(10) unsigned NOT NULL default '0',
  `n_dommatcheswon` int(10) unsigned NOT NULL default '0',
  `n_domstreak` int(10) unsigned NOT NULL default '0',
  `n_cwinstr` int(10) unsigned NOT NULL default '0',
  `n_bwinstr` int(10) unsigned NOT NULL default '0',
  `n_pptw` int(10) unsigned NOT NULL default '0',
  `n_pptwstreak` int(10) unsigned NOT NULL default '0',
  `n_pdas` int(10) unsigned NOT NULL default '0',
  `n_pdasstreak` int(10) unsigned NOT NULL default '0',
  `n_wbtd` int(10) unsigned NOT NULL default '0',
  `n_wbtdstreak` int(10) unsigned NOT NULL default '0',
  `n_ukills` int(10) unsigned NOT NULL default '0',
  `n_ulost` int(10) unsigned NOT NULL default '0',
  `n_cpc` int(10) unsigned NOT NULL default '0',
  `n_rps` int(10) unsigned NOT NULL default '0',
  `n_taps` int(10) unsigned NOT NULL default '0',
  `n_nukes` int(10) unsigned NOT NULL default '0',
  `n_nukestreak` int(10) unsigned NOT NULL default '0',
  `n_tach` int(10) unsigned NOT NULL default '0',
  `n_sause` int(10) unsigned NOT NULL default '0',
  `n_bplayer` int(10) unsigned NOT NULL default '0',
  `n_bplayerstr` int(10) unsigned NOT NULL default '0',
  `n_binf` int(10) unsigned NOT NULL default '0',
  `n_binfstr` int(10) unsigned NOT NULL default '0',
  `n_bsup` int(10) unsigned NOT NULL default '0',
  `n_bsupstr` int(10) unsigned NOT NULL default '0',
  `n_barm` int(10) unsigned NOT NULL default '0',
  `n_barmstr` int(10) unsigned NOT NULL default '0',
  `n_bair` int(10) unsigned NOT NULL default '0',
  `n_bairstr` int(10) unsigned NOT NULL default '0',
  `n_statpad` int(10) unsigned NOT NULL default '0',
  UNIQUE KEY `profileId` (`profileId`),
  KEY `sc_air` (`sc_air`),
  KEY `sc_inf` (`sc_inf`),
  KEY `sc_sup` (`sc_sup`),
  KEY `sc_arm` (`sc_arm`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `PlayerStatsTweakables`
--

DROP TABLE IF EXISTS `PlayerStatsTweakables`;
CREATE TABLE IF NOT EXISTS `PlayerStatsTweakables` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `aKey` varchar(32) default NULL,
  `aValue` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `ProductOwnership`
--

DROP TABLE IF EXISTS `ProductOwnership`;
CREATE TABLE IF NOT EXISTS `ProductOwnership` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `accountId` int(10) unsigned NOT NULL default '0',
  `productId` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `accountIdProductId` (`accountId`,`productId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Products`
--

DROP TABLE IF EXISTS `Products`;
CREATE TABLE IF NOT EXISTS `Products` (
  `productid` smallint(5) unsigned NOT NULL auto_increment,
  `productname` varchar(64) NOT NULL default '',
  UNIQUE KEY `productid` (`productid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `ProfileGuestbook`
--

DROP TABLE IF EXISTS `ProfileGuestbook`;
CREATE TABLE IF NOT EXISTS `ProfileGuestbook` (
  `id` int(11) NOT NULL auto_increment,
  `profileId` int(11) NOT NULL default '0',
  `posterProfileId` int(11) NOT NULL default '0',
  `timestamp` datetime NOT NULL default '0000-00-00 00:00:00',
  `message` varchar(128) NOT NULL default '',
  `isDeleted` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `ProfileIdIndex` (`profileId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `ProfileIgnoreList`
--

DROP TABLE IF EXISTS `ProfileIgnoreList`;
CREATE TABLE IF NOT EXISTS `ProfileIgnoreList` (
  `id` int(11) NOT NULL auto_increment,
  `profileId` int(10) unsigned NOT NULL default '0',
  `ignores` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `profileId` (`profileId`),
  KEY `ignores` (`ignores`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Profiles`
--

DROP TABLE IF EXISTS `Profiles`;
CREATE TABLE IF NOT EXISTS `Profiles` (
  `profileId` int(10) unsigned NOT NULL auto_increment,
  `accountId` int(10) unsigned NOT NULL default '0',
  `clanId` int(10) unsigned NOT NULL default '0',
  `rankInClan` tinyint(4) NOT NULL default '0',
  `profileName` varchar(22) NOT NULL default '',
  `normalizedProfileName` varchar(32) NOT NULL default '',
  `lastLogin` datetime NOT NULL default '0000-00-00 00:00:00',
  `isDeleted` enum('no','yes') NOT NULL default 'no',
  `groupMembership` int(10) unsigned NOT NULL default '0',
  `communicationOptions` int(1) unsigned NOT NULL default '9215',
  `homepage` varchar(255) NOT NULL default '',
  `motto` varchar(255) NOT NULL default '',
  PRIMARY KEY  (`profileId`),
  UNIQUE KEY `normalizedProfileName` (`normalizedProfileName`),
  KEY `accountId` (`accountId`),
  KEY `clanId` (`clanId`),
  KEY `isDeleted` (`isDeleted`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `ServerStatistics`
--

DROP TABLE IF EXISTS `ServerStatistics`;
CREATE TABLE IF NOT EXISTS `ServerStatistics` (
  `anId` int(10) unsigned NOT NULL auto_increment,
  `aKey` varchar(128) NOT NULL default '',
  `aValue` int(11) NOT NULL default '0',
  `aTimestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  PRIMARY KEY  (`anId`),
  KEY `aTimestamp` (`aTimestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Settings`
--

DROP TABLE IF EXISTS `Settings`;
CREATE TABLE IF NOT EXISTS `Settings` (
  `aVariable` varchar(64) NOT NULL default '',
  `aValue` varchar(255) NOT NULL default '',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  UNIQUE KEY `aVariable` (`aVariable`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `SuspectStats`
--

DROP TABLE IF EXISTS `SuspectStats`;
CREATE TABLE IF NOT EXISTS `SuspectStats` (
  `profileId` int(10) unsigned NOT NULL default '0',
  `rank` tinyint(3) unsigned NOT NULL default '0',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `sc_tot` int(11) NOT NULL default '0',
  `sc_highest` int(11) NOT NULL default '0',
  `sc_inf` int(11) NOT NULL default '0',
  `sc_highinf` int(11) NOT NULL default '0',
  `sc_sup` int(11) NOT NULL default '0',
  `sc_highsup` int(11) NOT NULL default '0',
  `sc_arm` int(11) NOT NULL default '0',
  `sc_higharm` int(11) NOT NULL default '0',
  `sc_air` int(11) NOT NULL default '0',
  `sc_highair` int(11) NOT NULL default '0',
  `sc_damen` int(11) NOT NULL default '0',
  `sc_ta` int(11) NOT NULL default '0',
  `sc_cpc` int(11) NOT NULL default '0',
  `sc_rep` int(11) NOT NULL default '0',
  `sc_fort` int(11) NOT NULL default '0',
  `sc_tk` int(11) NOT NULL default '0',
  `t_USA` int(10) unsigned NOT NULL default '0',
  `t_USSR` int(10) unsigned NOT NULL default '0',
  `t_NATO` int(10) unsigned NOT NULL default '0',
  `t_sup` int(10) unsigned NOT NULL default '0',
  `t_arm` int(10) unsigned NOT NULL default '0',
  `t_air` int(10) unsigned NOT NULL default '0',
  `t_inf` int(10) unsigned NOT NULL default '0',
  `n_ukills` int(10) unsigned NOT NULL default '0',
  `n_ulost` int(10) unsigned NOT NULL default '0',
  `n_cpc` int(10) unsigned NOT NULL default '0',
  `n_rps` int(10) unsigned NOT NULL default '0',
  `n_taps` int(10) unsigned NOT NULL default '0',
  `n_nukes` int(10) unsigned NOT NULL default '0',
  `n_tach` int(10) unsigned NOT NULL default '0',
  KEY `profileId` (`profileId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `TournamentMatches`
--

DROP TABLE IF EXISTS `TournamentMatches`;
CREATE TABLE IF NOT EXISTS `TournamentMatches` (
  `matchId` int(10) unsigned NOT NULL auto_increment,
  `parentMatch` int(10) unsigned NOT NULL default '0',
  `round` int(10) unsigned NOT NULL default '0',
  `tournamentId` int(10) unsigned NOT NULL default '0',
  `clanA` int(10) unsigned NOT NULL default '0',
  `clanB` int(10) unsigned NOT NULL default '0',
  `time` datetime NOT NULL default '0000-00-00 00:00:00',
  `winner` int(10) unsigned NOT NULL default '0',
  `currentWinner` int(10) unsigned NOT NULL default '0' COMMENT 'Current match leader (team)',
  `gameTime` int(10) unsigned NOT NULL default '0',
  `map` bigint(20) unsigned NOT NULL default '0',
  `serverId` int(10) unsigned NOT NULL default '0',
  `eventExpire` datetime NOT NULL default '0000-00-00 00:00:00',
  `password` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`matchId`),
  KEY `tournamentId` (`tournamentId`,`clanA`,`clanB`),
  KEY `time` (`time`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Tournaments`
--

DROP TABLE IF EXISTS `Tournaments`;
CREATE TABLE IF NOT EXISTS `Tournaments` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `name` varchar(32) NOT NULL default '',
  `maxTeamSize` tinyint(4) NOT NULL default '0',
  `minRank` tinyint(3) unsigned NOT NULL default '0',
  `signupBy` datetime NOT NULL default '0000-00-00 00:00:00',
  `inviteOnly` tinyint(3) unsigned NOT NULL default '0',
  `maxNumTeams` int(11) NOT NULL default '0',
  `maxMatchTime` tinyint(3) unsigned NOT NULL default '20',
  `status` enum('scheduled','ongoing','completed','hidden') NOT NULL default 'scheduled',
  `tournamentType` enum('varied','domination','assault','koth') NOT NULL default 'varied',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `TournamentTeams`
--

DROP TABLE IF EXISTS `TournamentTeams`;
CREATE TABLE IF NOT EXISTS `TournamentTeams` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `tournamentId` int(10) unsigned NOT NULL default '0',
  `clanId` int(10) unsigned NOT NULL default '0',
  `clanName` varchar(32) character set utf8 NOT NULL default '',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `state` enum('invited','applied','accepted','rejected','left','processing') NOT NULL default 'applied',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `tournamentId_2` (`tournamentId`,`clanId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `WICCycleHashList`
--

DROP TABLE IF EXISTS `WICCycleHashList`;
CREATE TABLE IF NOT EXISTS `WICCycleHashList` (
  `cycleHash` bigint(20) unsigned NOT NULL default '0',
  `mapHash` bigint(20) unsigned NOT NULL default '0',
  `count` int(10) unsigned NOT NULL default '1',
  `updated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`cycleHash`,`mapHash`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `WICEventTournament`
--

DROP TABLE IF EXISTS `WICEventTournament`;
CREATE TABLE IF NOT EXISTS `WICEventTournament` (
  `teventID` int(10) unsigned NOT NULL auto_increment,
  `gameStartTime` datetime NOT NULL default '0000-00-00 00:00:00',
  `expire` datetime NOT NULL default '0000-00-00 00:00:00',
  `tournamentID` int(11) NOT NULL default '0',
  `tournamentMatchID` int(11) NOT NULL default '0',
  `mapHash` bigint(20) unsigned NOT NULL default '0',
  `serverId` int(10) unsigned NOT NULL default '0',
  `updated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  PRIMARY KEY  (`teventID`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `WICRankDefinitions`
--

DROP TABLE IF EXISTS `WICRankDefinitions`;
CREATE TABLE IF NOT EXISTS `WICRankDefinitions` (
  `id` int(10) unsigned NOT NULL default '0',
  `totalScore` int(10) unsigned NOT NULL default '0',
  `ladderPercentage` int(11) NOT NULL default '0',
  `db_description` varchar(32) NOT NULL default '',
  UNIQUE KEY `id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `WICServers`
--

DROP TABLE IF EXISTS `WICServers`;
CREATE TABLE IF NOT EXISTS `WICServers` (
  `serverId` int(10) unsigned NOT NULL auto_increment,
  `serverName` varchar(128) NOT NULL default '',
  `serverType` enum('NORMAL','MATCH','FPM','TOURNAMENT','CLANMATCH') NOT NULL default 'NORMAL',
  `publicIp` varchar(16) character set latin1 NOT NULL default '0',
  `privateIp` varchar(16) character set latin1 NOT NULL default '',
  `isDedicated` tinyint(1) unsigned NOT NULL default '0',
  `isRanked` tinyint(1) unsigned NOT NULL default '0',
  `matchTimeExpire` datetime NOT NULL default '2000-01-01 00:00:00',
  `matchId` int(11) unsigned NOT NULL default '0',
  `serverReliablePort` int(10) unsigned NOT NULL default '0',
  `massgateCommPort` int(10) unsigned NOT NULL default '0',
  `numPlayers` smallint(5) unsigned NOT NULL default '0',
  `maxPlayers` smallint(5) unsigned NOT NULL default '0',
  `cycleHash` bigint(20) unsigned NOT NULL default '0',
  `currentMap` bigint(20) unsigned NOT NULL default '0',
  `modId` int(10) unsigned NOT NULL default '0',
  `voipEnabled` tinyint(4) NOT NULL default '0',
  `gameVersion` int(16) unsigned NOT NULL default '0',
  `protocolVersion` int(10) unsigned NOT NULL default '0',
  `isPasswordProtected` tinyint(1) unsigned NOT NULL default '0',
  `startTime` datetime NOT NULL default '0000-00-00 00:00:00',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `gameTime` float NOT NULL default '0',
  `accessControl` varchar(255) character set latin1 NOT NULL default 'allow: * except: banned',
  `userConnectCookie` int(10) unsigned NOT NULL default '0',
  `fingerprint` int(10) unsigned NOT NULL default '0',
  `s2i` int(10) unsigned NOT NULL default '0',
  `hostProfileId` int(10) unsigned NOT NULL default '0',
  `isOnline` tinyint(4) NOT NULL default '1',
  `usingCdKey` int(10) unsigned NOT NULL default '0',
  `continent` varchar(4) NOT NULL default '',
  `country` varchar(4) NOT NULL default '',
  PRIMARY KEY  (`serverId`),
  KEY `lastUpdated` (`lastUpdated`),
  KEY `isOnline` (`isOnline`),
  KEY `continent` (`continent`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_archive`
--

DROP TABLE IF EXISTS `Wiki_archive`;
CREATE TABLE IF NOT EXISTS `Wiki_archive` (
  `ar_namespace` int(11) NOT NULL default '0',
  `ar_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `ar_text` mediumblob NOT NULL,
  `ar_comment` tinyblob NOT NULL,
  `ar_user` int(10) unsigned NOT NULL default '0',
  `ar_user_text` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `ar_timestamp` varbinary(14) NOT NULL default '',
  `ar_minor_edit` tinyint(4) NOT NULL default '0',
  `ar_flags` tinyblob NOT NULL,
  `ar_rev_id` int(10) unsigned default NULL,
  `ar_text_id` int(10) unsigned default NULL,
  `ar_deleted` tinyint(3) unsigned NOT NULL default '0',
  `ar_len` int(10) unsigned default NULL,
  `ar_page_id` int(10) unsigned default NULL,
  KEY `name_title_timestamp` (`ar_namespace`,`ar_title`,`ar_timestamp`),
  KEY `usertext_timestamp` (`ar_user_text`,`ar_timestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_categorylinks`
--

DROP TABLE IF EXISTS `Wiki_categorylinks`;
CREATE TABLE IF NOT EXISTS `Wiki_categorylinks` (
  `cl_from` int(10) unsigned NOT NULL default '0',
  `cl_to` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `cl_sortkey` varchar(70) character set latin1 collate latin1_bin NOT NULL default '',
  `cl_timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  UNIQUE KEY `cl_from` (`cl_from`,`cl_to`),
  KEY `cl_sortkey` (`cl_to`,`cl_sortkey`,`cl_from`),
  KEY `cl_timestamp` (`cl_to`,`cl_timestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_externallinks`
--

DROP TABLE IF EXISTS `Wiki_externallinks`;
CREATE TABLE IF NOT EXISTS `Wiki_externallinks` (
  `el_from` int(10) unsigned NOT NULL default '0',
  `el_to` blob NOT NULL,
  `el_index` blob NOT NULL,
  KEY `el_from` (`el_from`,`el_to`(40)),
  KEY `el_to` (`el_to`(60),`el_from`),
  KEY `el_index` (`el_index`(60))
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_filearchive`
--

DROP TABLE IF EXISTS `Wiki_filearchive`;
CREATE TABLE IF NOT EXISTS `Wiki_filearchive` (
  `fa_id` int(11) NOT NULL auto_increment,
  `fa_name` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `fa_archive_name` varchar(255) character set latin1 collate latin1_bin default '',
  `fa_storage_group` varbinary(16) default NULL,
  `fa_storage_key` varbinary(64) default '',
  `fa_deleted_user` int(11) default NULL,
  `fa_deleted_timestamp` varbinary(14) default '',
  `fa_deleted_reason` text,
  `fa_size` int(10) unsigned default '0',
  `fa_width` int(11) default '0',
  `fa_height` int(11) default '0',
  `fa_metadata` mediumblob,
  `fa_bits` int(11) default '0',
  `fa_media_type` enum('UNKNOWN','BITMAP','DRAWING','AUDIO','VIDEO','MULTIMEDIA','OFFICE','TEXT','EXECUTABLE','ARCHIVE') default NULL,
  `fa_major_mime` enum('unknown','application','audio','image','text','video','message','model','multipart') default 'unknown',
  `fa_minor_mime` varbinary(32) default 'unknown',
  `fa_description` tinyblob,
  `fa_user` int(10) unsigned default '0',
  `fa_user_text` varchar(255) character set latin1 collate latin1_bin default NULL,
  `fa_timestamp` varbinary(14) default '',
  `fa_deleted` tinyint(3) unsigned NOT NULL default '0',
  PRIMARY KEY  (`fa_id`),
  KEY `fa_name` (`fa_name`,`fa_timestamp`),
  KEY `fa_storage_group` (`fa_storage_group`,`fa_storage_key`),
  KEY `fa_deleted_timestamp` (`fa_deleted_timestamp`),
  KEY `fa_deleted_user` (`fa_deleted_user`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_hitcounter`
--

DROP TABLE IF EXISTS `Wiki_hitcounter`;
CREATE TABLE IF NOT EXISTS `Wiki_hitcounter` (
  `hc_id` int(10) unsigned NOT NULL default '0'
) ENGINE=HEAP DEFAULT CHARSET=latin1 MAX_ROWS=25000;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_image`
--

DROP TABLE IF EXISTS `Wiki_image`;
CREATE TABLE IF NOT EXISTS `Wiki_image` (
  `img_name` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `img_size` int(10) unsigned NOT NULL default '0',
  `img_width` int(11) NOT NULL default '0',
  `img_height` int(11) NOT NULL default '0',
  `img_metadata` mediumblob NOT NULL,
  `img_bits` int(11) NOT NULL default '0',
  `img_media_type` enum('UNKNOWN','BITMAP','DRAWING','AUDIO','VIDEO','MULTIMEDIA','OFFICE','TEXT','EXECUTABLE','ARCHIVE') default NULL,
  `img_major_mime` enum('unknown','application','audio','image','text','video','message','model','multipart') NOT NULL default 'unknown',
  `img_minor_mime` varbinary(32) NOT NULL default 'unknown',
  `img_description` tinyblob NOT NULL,
  `img_user` int(10) unsigned NOT NULL default '0',
  `img_user_text` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `img_timestamp` varbinary(14) NOT NULL default '',
  `img_sha1` varbinary(32) NOT NULL default '',
  PRIMARY KEY  (`img_name`),
  KEY `img_usertext_timestamp` (`img_user_text`,`img_timestamp`),
  KEY `img_size` (`img_size`),
  KEY `img_timestamp` (`img_timestamp`),
  KEY `img_sha1` (`img_sha1`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_imagelinks`
--

DROP TABLE IF EXISTS `Wiki_imagelinks`;
CREATE TABLE IF NOT EXISTS `Wiki_imagelinks` (
  `il_from` int(10) unsigned NOT NULL default '0',
  `il_to` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  UNIQUE KEY `il_from` (`il_from`,`il_to`),
  KEY `il_to` (`il_to`,`il_from`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_interwiki`
--

DROP TABLE IF EXISTS `Wiki_interwiki`;
CREATE TABLE IF NOT EXISTS `Wiki_interwiki` (
  `iw_prefix` varchar(32) NOT NULL default '',
  `iw_url` blob NOT NULL,
  `iw_local` tinyint(1) NOT NULL default '0',
  `iw_trans` tinyint(4) NOT NULL default '0',
  UNIQUE KEY `iw_prefix` (`iw_prefix`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_ipblocks`
--

DROP TABLE IF EXISTS `Wiki_ipblocks`;
CREATE TABLE IF NOT EXISTS `Wiki_ipblocks` (
  `ipb_id` int(11) NOT NULL auto_increment,
  `ipb_address` tinyblob NOT NULL,
  `ipb_user` int(10) unsigned NOT NULL default '0',
  `ipb_by` int(10) unsigned NOT NULL default '0',
  `ipb_reason` tinyblob NOT NULL,
  `ipb_timestamp` varbinary(14) NOT NULL default '',
  `ipb_auto` tinyint(1) NOT NULL default '0',
  `ipb_anon_only` tinyint(1) NOT NULL default '0',
  `ipb_create_account` tinyint(1) NOT NULL default '1',
  `ipb_enable_autoblock` tinyint(1) NOT NULL default '1',
  `ipb_expiry` varbinary(14) NOT NULL default '',
  `ipb_range_start` tinyblob NOT NULL,
  `ipb_range_end` tinyblob NOT NULL,
  `ipb_deleted` tinyint(1) NOT NULL default '0',
  `ipb_block_email` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`ipb_id`),
  UNIQUE KEY `ipb_address` (`ipb_address`(255),`ipb_user`,`ipb_auto`,`ipb_anon_only`),
  KEY `ipb_user` (`ipb_user`),
  KEY `ipb_range` (`ipb_range_start`(8),`ipb_range_end`(8)),
  KEY `ipb_timestamp` (`ipb_timestamp`),
  KEY `ipb_expiry` (`ipb_expiry`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_job`
--

DROP TABLE IF EXISTS `Wiki_job`;
CREATE TABLE IF NOT EXISTS `Wiki_job` (
  `job_id` int(10) unsigned NOT NULL auto_increment,
  `job_cmd` varbinary(60) NOT NULL default '',
  `job_namespace` int(11) NOT NULL default '0',
  `job_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `job_params` blob NOT NULL,
  PRIMARY KEY  (`job_id`),
  KEY `job_cmd` (`job_cmd`,`job_namespace`,`job_title`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_langlinks`
--

DROP TABLE IF EXISTS `Wiki_langlinks`;
CREATE TABLE IF NOT EXISTS `Wiki_langlinks` (
  `ll_from` int(10) unsigned NOT NULL default '0',
  `ll_lang` varbinary(20) NOT NULL default '',
  `ll_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  UNIQUE KEY `ll_from` (`ll_from`,`ll_lang`),
  KEY `ll_lang` (`ll_lang`,`ll_title`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_logging`
--

DROP TABLE IF EXISTS `Wiki_logging`;
CREATE TABLE IF NOT EXISTS `Wiki_logging` (
  `log_type` varbinary(10) NOT NULL default '',
  `log_action` varbinary(10) NOT NULL default '',
  `log_timestamp` varbinary(14) NOT NULL default '19700101000000',
  `log_user` int(10) unsigned NOT NULL default '0',
  `log_namespace` int(11) NOT NULL default '0',
  `log_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `log_comment` varchar(255) NOT NULL default '',
  `log_params` blob NOT NULL,
  `log_id` int(10) unsigned NOT NULL auto_increment,
  `log_deleted` tinyint(3) unsigned NOT NULL default '0',
  PRIMARY KEY  (`log_id`),
  KEY `type_time` (`log_type`,`log_timestamp`),
  KEY `user_time` (`log_user`,`log_timestamp`),
  KEY `page_time` (`log_namespace`,`log_title`,`log_timestamp`),
  KEY `times` (`log_timestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_math`
--

DROP TABLE IF EXISTS `Wiki_math`;
CREATE TABLE IF NOT EXISTS `Wiki_math` (
  `math_inputhash` varbinary(16) NOT NULL default '',
  `math_outputhash` varbinary(16) NOT NULL default '',
  `math_html_conservativeness` tinyint(4) NOT NULL default '0',
  `math_html` text,
  `math_mathml` text,
  UNIQUE KEY `math_inputhash` (`math_inputhash`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_objectcache`
--

DROP TABLE IF EXISTS `Wiki_objectcache`;
CREATE TABLE IF NOT EXISTS `Wiki_objectcache` (
  `keyname` varbinary(255) NOT NULL default '',
  `value` mediumblob,
  `exptime` datetime default NULL,
  UNIQUE KEY `keyname` (`keyname`),
  KEY `exptime` (`exptime`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_oldimage`
--

DROP TABLE IF EXISTS `Wiki_oldimage`;
CREATE TABLE IF NOT EXISTS `Wiki_oldimage` (
  `oi_name` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `oi_archive_name` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `oi_size` int(10) unsigned NOT NULL default '0',
  `oi_width` int(11) NOT NULL default '0',
  `oi_height` int(11) NOT NULL default '0',
  `oi_bits` int(11) NOT NULL default '0',
  `oi_description` tinyblob NOT NULL,
  `oi_user` int(10) unsigned NOT NULL default '0',
  `oi_user_text` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `oi_timestamp` varbinary(14) NOT NULL default '',
  `oi_metadata` mediumblob NOT NULL,
  `oi_media_type` enum('UNKNOWN','BITMAP','DRAWING','AUDIO','VIDEO','MULTIMEDIA','OFFICE','TEXT','EXECUTABLE','ARCHIVE') default NULL,
  `oi_major_mime` enum('unknown','application','audio','image','text','video','message','model','multipart') NOT NULL default 'unknown',
  `oi_minor_mime` varbinary(32) NOT NULL default 'unknown',
  `oi_deleted` tinyint(3) unsigned NOT NULL default '0',
  `oi_sha1` varbinary(32) NOT NULL default '',
  KEY `oi_usertext_timestamp` (`oi_user_text`,`oi_timestamp`),
  KEY `oi_name_timestamp` (`oi_name`,`oi_timestamp`),
  KEY `oi_name_archive_name` (`oi_name`,`oi_archive_name`(14)),
  KEY `oi_sha1` (`oi_sha1`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_page`
--

DROP TABLE IF EXISTS `Wiki_page`;
CREATE TABLE IF NOT EXISTS `Wiki_page` (
  `page_id` int(10) unsigned NOT NULL auto_increment,
  `page_namespace` int(11) NOT NULL default '0',
  `page_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `page_restrictions` tinyblob NOT NULL,
  `page_counter` bigint(20) unsigned NOT NULL default '0',
  `page_is_redirect` tinyint(3) unsigned NOT NULL default '0',
  `page_is_new` tinyint(3) unsigned NOT NULL default '0',
  `page_random` double unsigned NOT NULL default '0',
  `page_touched` varbinary(14) NOT NULL default '',
  `page_latest` int(10) unsigned NOT NULL default '0',
  `page_len` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`page_id`),
  UNIQUE KEY `name_title` (`page_namespace`,`page_title`),
  KEY `page_random` (`page_random`),
  KEY `page_len` (`page_len`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_pagelinks`
--

DROP TABLE IF EXISTS `Wiki_pagelinks`;
CREATE TABLE IF NOT EXISTS `Wiki_pagelinks` (
  `pl_from` int(10) unsigned NOT NULL default '0',
  `pl_namespace` int(11) NOT NULL default '0',
  `pl_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  UNIQUE KEY `pl_from` (`pl_from`,`pl_namespace`,`pl_title`),
  KEY `pl_namespace` (`pl_namespace`,`pl_title`,`pl_from`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_page_restrictions`
--

DROP TABLE IF EXISTS `Wiki_page_restrictions`;
CREATE TABLE IF NOT EXISTS `Wiki_page_restrictions` (
  `pr_page` int(11) NOT NULL default '0',
  `pr_type` varbinary(60) NOT NULL default '',
  `pr_level` varbinary(60) NOT NULL default '',
  `pr_cascade` tinyint(4) NOT NULL default '0',
  `pr_user` int(11) default NULL,
  `pr_expiry` varbinary(14) default NULL,
  `pr_id` int(10) unsigned NOT NULL auto_increment,
  PRIMARY KEY  (`pr_page`,`pr_type`),
  UNIQUE KEY `pr_id` (`pr_id`),
  KEY `pr_typelevel` (`pr_type`,`pr_level`),
  KEY `pr_level` (`pr_level`),
  KEY `pr_cascade` (`pr_cascade`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_querycache`
--

DROP TABLE IF EXISTS `Wiki_querycache`;
CREATE TABLE IF NOT EXISTS `Wiki_querycache` (
  `qc_type` varbinary(32) NOT NULL default '',
  `qc_value` int(10) unsigned NOT NULL default '0',
  `qc_namespace` int(11) NOT NULL default '0',
  `qc_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  KEY `qc_type` (`qc_type`,`qc_value`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_querycachetwo`
--

DROP TABLE IF EXISTS `Wiki_querycachetwo`;
CREATE TABLE IF NOT EXISTS `Wiki_querycachetwo` (
  `qcc_type` varbinary(32) NOT NULL default '',
  `qcc_value` int(10) unsigned NOT NULL default '0',
  `qcc_namespace` int(11) NOT NULL default '0',
  `qcc_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `qcc_namespacetwo` int(11) NOT NULL default '0',
  `qcc_titletwo` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  KEY `qcc_type` (`qcc_type`,`qcc_value`),
  KEY `qcc_title` (`qcc_type`,`qcc_namespace`,`qcc_title`),
  KEY `qcc_titletwo` (`qcc_type`,`qcc_namespacetwo`,`qcc_titletwo`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_querycache_info`
--

DROP TABLE IF EXISTS `Wiki_querycache_info`;
CREATE TABLE IF NOT EXISTS `Wiki_querycache_info` (
  `qci_type` varbinary(32) NOT NULL default '',
  `qci_timestamp` varbinary(14) NOT NULL default '19700101000000',
  UNIQUE KEY `qci_type` (`qci_type`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_recentchanges`
--

DROP TABLE IF EXISTS `Wiki_recentchanges`;
CREATE TABLE IF NOT EXISTS `Wiki_recentchanges` (
  `rc_id` int(11) NOT NULL auto_increment,
  `rc_timestamp` varbinary(14) NOT NULL default '',
  `rc_cur_time` varbinary(14) NOT NULL default '',
  `rc_user` int(10) unsigned NOT NULL default '0',
  `rc_user_text` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `rc_namespace` int(11) NOT NULL default '0',
  `rc_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `rc_comment` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `rc_minor` tinyint(3) unsigned NOT NULL default '0',
  `rc_bot` tinyint(3) unsigned NOT NULL default '0',
  `rc_new` tinyint(3) unsigned NOT NULL default '0',
  `rc_cur_id` int(10) unsigned NOT NULL default '0',
  `rc_this_oldid` int(10) unsigned NOT NULL default '0',
  `rc_last_oldid` int(10) unsigned NOT NULL default '0',
  `rc_type` tinyint(3) unsigned NOT NULL default '0',
  `rc_moved_to_ns` tinyint(3) unsigned NOT NULL default '0',
  `rc_moved_to_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `rc_patrolled` tinyint(3) unsigned NOT NULL default '0',
  `rc_ip` varbinary(40) NOT NULL default '',
  `rc_old_len` int(11) default NULL,
  `rc_new_len` int(11) default NULL,
  `rc_deleted` tinyint(3) unsigned NOT NULL default '0',
  `rc_logid` int(10) unsigned NOT NULL default '0',
  `rc_log_type` varbinary(255) default NULL,
  `rc_log_action` varbinary(255) default NULL,
  `rc_params` blob,
  PRIMARY KEY  (`rc_id`),
  KEY `rc_timestamp` (`rc_timestamp`),
  KEY `rc_namespace_title` (`rc_namespace`,`rc_title`),
  KEY `rc_cur_id` (`rc_cur_id`),
  KEY `new_name_timestamp` (`rc_new`,`rc_namespace`,`rc_timestamp`),
  KEY `rc_ip` (`rc_ip`),
  KEY `rc_ns_usertext` (`rc_namespace`,`rc_user_text`),
  KEY `rc_user_text` (`rc_user_text`,`rc_timestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_redirect`
--

DROP TABLE IF EXISTS `Wiki_redirect`;
CREATE TABLE IF NOT EXISTS `Wiki_redirect` (
  `rd_from` int(10) unsigned NOT NULL default '0',
  `rd_namespace` int(11) NOT NULL default '0',
  `rd_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  PRIMARY KEY  (`rd_from`),
  KEY `rd_ns_title` (`rd_namespace`,`rd_title`,`rd_from`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_revision`
--

DROP TABLE IF EXISTS `Wiki_revision`;
CREATE TABLE IF NOT EXISTS `Wiki_revision` (
  `rev_id` int(10) unsigned NOT NULL auto_increment,
  `rev_page` int(10) unsigned NOT NULL default '0',
  `rev_text_id` int(10) unsigned NOT NULL default '0',
  `rev_comment` tinyblob NOT NULL,
  `rev_user` int(10) unsigned NOT NULL default '0',
  `rev_user_text` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `rev_timestamp` varbinary(14) NOT NULL default '',
  `rev_minor_edit` tinyint(3) unsigned NOT NULL default '0',
  `rev_deleted` tinyint(3) unsigned NOT NULL default '0',
  `rev_len` int(10) unsigned default NULL,
  `rev_parent_id` int(10) unsigned default NULL,
  PRIMARY KEY  (`rev_page`,`rev_id`),
  UNIQUE KEY `rev_id` (`rev_id`),
  KEY `rev_timestamp` (`rev_timestamp`),
  KEY `page_timestamp` (`rev_page`,`rev_timestamp`),
  KEY `user_timestamp` (`rev_user`,`rev_timestamp`),
  KEY `usertext_timestamp` (`rev_user_text`,`rev_timestamp`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 MAX_ROWS=10000000 AVG_ROW_LENGTH=1024;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_searchindex`
--

DROP TABLE IF EXISTS `Wiki_searchindex`;
CREATE TABLE IF NOT EXISTS `Wiki_searchindex` (
  `si_page` int(10) unsigned NOT NULL default '0',
  `si_title` varchar(255) NOT NULL default '',
  `si_text` mediumtext NOT NULL,
  UNIQUE KEY `si_page` (`si_page`),
  FULLTEXT KEY `si_title` (`si_title`),
  FULLTEXT KEY `si_text` (`si_text`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_site_stats`
--

DROP TABLE IF EXISTS `Wiki_site_stats`;
CREATE TABLE IF NOT EXISTS `Wiki_site_stats` (
  `ss_row_id` int(10) unsigned NOT NULL default '0',
  `ss_total_views` bigint(20) unsigned default '0',
  `ss_total_edits` bigint(20) unsigned default '0',
  `ss_good_articles` bigint(20) unsigned default '0',
  `ss_total_pages` bigint(20) default '-1',
  `ss_users` bigint(20) default '-1',
  `ss_admins` int(11) default '-1',
  `ss_images` int(11) default '0',
  UNIQUE KEY `ss_row_id` (`ss_row_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_templatelinks`
--

DROP TABLE IF EXISTS `Wiki_templatelinks`;
CREATE TABLE IF NOT EXISTS `Wiki_templatelinks` (
  `tl_from` int(10) unsigned NOT NULL default '0',
  `tl_namespace` int(11) NOT NULL default '0',
  `tl_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  UNIQUE KEY `tl_from` (`tl_from`,`tl_namespace`,`tl_title`),
  KEY `tl_namespace` (`tl_namespace`,`tl_title`,`tl_from`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_text`
--

DROP TABLE IF EXISTS `Wiki_text`;
CREATE TABLE IF NOT EXISTS `Wiki_text` (
  `old_id` int(10) unsigned NOT NULL auto_increment,
  `old_text` mediumblob NOT NULL,
  `old_flags` tinyblob NOT NULL,
  PRIMARY KEY  (`old_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 MAX_ROWS=10000000 AVG_ROW_LENGTH=10240;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_trackbacks`
--

DROP TABLE IF EXISTS `Wiki_trackbacks`;
CREATE TABLE IF NOT EXISTS `Wiki_trackbacks` (
  `tb_id` int(11) NOT NULL auto_increment,
  `tb_page` int(11) default NULL,
  `tb_title` varchar(255) NOT NULL default '',
  `tb_url` blob NOT NULL,
  `tb_ex` text,
  `tb_name` varchar(255) default NULL,
  PRIMARY KEY  (`tb_id`),
  KEY `tb_page` (`tb_page`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_transcache`
--

DROP TABLE IF EXISTS `Wiki_transcache`;
CREATE TABLE IF NOT EXISTS `Wiki_transcache` (
  `tc_url` varbinary(255) NOT NULL default '',
  `tc_contents` text,
  `tc_time` int(11) NOT NULL default '0',
  UNIQUE KEY `tc_url_idx` (`tc_url`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_user`
--

DROP TABLE IF EXISTS `Wiki_user`;
CREATE TABLE IF NOT EXISTS `Wiki_user` (
  `user_id` int(10) unsigned NOT NULL auto_increment,
  `user_name` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `user_real_name` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `user_password` tinyblob NOT NULL,
  `user_newpassword` tinyblob NOT NULL,
  `user_newpass_time` varbinary(14) default NULL,
  `user_email` tinytext NOT NULL,
  `user_options` blob NOT NULL,
  `user_touched` varbinary(14) NOT NULL default '',
  `user_token` varbinary(32) NOT NULL default '',
  `user_email_authenticated` varbinary(14) default NULL,
  `user_email_token` varbinary(32) default NULL,
  `user_email_token_expires` varbinary(14) default NULL,
  `user_registration` varbinary(14) default NULL,
  `user_editcount` int(11) default NULL,
  PRIMARY KEY  (`user_id`),
  UNIQUE KEY `user_name` (`user_name`),
  KEY `user_email_token` (`user_email_token`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_user_groups`
--

DROP TABLE IF EXISTS `Wiki_user_groups`;
CREATE TABLE IF NOT EXISTS `Wiki_user_groups` (
  `ug_user` int(10) unsigned NOT NULL default '0',
  `ug_group` varbinary(16) NOT NULL default '',
  PRIMARY KEY  (`ug_user`,`ug_group`),
  KEY `ug_group` (`ug_group`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_user_newtalk`
--

DROP TABLE IF EXISTS `Wiki_user_newtalk`;
CREATE TABLE IF NOT EXISTS `Wiki_user_newtalk` (
  `user_id` int(11) NOT NULL default '0',
  `user_ip` varbinary(40) NOT NULL default '',
  KEY `user_id` (`user_id`),
  KEY `user_ip` (`user_ip`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `Wiki_watchlist`
--

DROP TABLE IF EXISTS `Wiki_watchlist`;
CREATE TABLE IF NOT EXISTS `Wiki_watchlist` (
  `wl_user` int(10) unsigned NOT NULL default '0',
  `wl_namespace` int(11) NOT NULL default '0',
  `wl_title` varchar(255) character set latin1 collate latin1_bin NOT NULL default '',
  `wl_notificationtimestamp` varbinary(14) default NULL,
  UNIQUE KEY `wl_user` (`wl_user`,`wl_namespace`,`wl_title`),
  KEY `namespace_title` (`wl_namespace`,`wl_title`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_DontKeepAccounts`
--

DROP TABLE IF EXISTS `_DontKeepAccounts`;
CREATE TABLE IF NOT EXISTS `_DontKeepAccounts` (
  `accountId` int(10) unsigned NOT NULL default '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_DontKeepClans`
--

DROP TABLE IF EXISTS `_DontKeepClans`;
CREATE TABLE IF NOT EXISTS `_DontKeepClans` (
  `clanId` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`clanId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_DontKeepProfiles`
--

DROP TABLE IF EXISTS `_DontKeepProfiles`;
CREATE TABLE IF NOT EXISTS `_DontKeepProfiles` (
  `profileId` int(10) unsigned NOT NULL default '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_invalidatecafe`
--

DROP TABLE IF EXISTS `_invalidatecafe`;
CREATE TABLE IF NOT EXISTS `_invalidatecafe` (
  `id` int(11) NOT NULL auto_increment,
  `cafeId` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `_InvalidateClanStats`
--

DROP TABLE IF EXISTS `_InvalidateClanStats`;
CREATE TABLE IF NOT EXISTS `_InvalidateClanStats` (
  `id` int(11) NOT NULL auto_increment,
  `clanId` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `_InvalidateLadder`
--

DROP TABLE IF EXISTS `_InvalidateLadder`;
CREATE TABLE IF NOT EXISTS `_InvalidateLadder` (
  `id` int(11) NOT NULL auto_increment,
  `inTableId` int(11) default '0',
  `tableName` enum('PlayerLadder','ClanLadder','PlayerVsPlayerLadder','BestOfLadder') default NULL,
  UNIQUE KEY `id` (`id`,`inTableId`,`tableName`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `_InvalidateLuts`
--

DROP TABLE IF EXISTS `_InvalidateLuts`;
CREATE TABLE IF NOT EXISTS `_InvalidateLuts` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `profileId` int(10) unsigned NOT NULL default '0',
  `reason` enum('none','stats') NOT NULL default 'none',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `profileId` (`profileId`)
) ENGINE=HEAP  DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_KeepAccounts`
--

DROP TABLE IF EXISTS `_KeepAccounts`;
CREATE TABLE IF NOT EXISTS `_KeepAccounts` (
  `accountId` int(10) unsigned NOT NULL default '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_KeepClans`
--

DROP TABLE IF EXISTS `_KeepClans`;
CREATE TABLE IF NOT EXISTS `_KeepClans` (
  `clanId` int(10) unsigned NOT NULL default '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_KeepProfiles`
--

DROP TABLE IF EXISTS `_KeepProfiles`;
CREATE TABLE IF NOT EXISTS `_KeepProfiles` (
  `profileId` int(10) unsigned NOT NULL default '0'
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_MassgateAlive`
--

DROP TABLE IF EXISTS `_MassgateAlive`;
CREATE TABLE IF NOT EXISTS `_MassgateAlive` (
  `ServerIP` varchar(32) NOT NULL default '',
  `ServerPort` int(10) unsigned NOT NULL default '0',
  `type` varchar(32) NOT NULL default '',
  `lastSeen` datetime NOT NULL default '0000-00-00 00:00:00',
  `threadId` int(10) NOT NULL default '0',
  PRIMARY KEY  (`ServerIP`,`ServerPort`,`type`,`threadId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_MemberSince`
--

DROP TABLE IF EXISTS `_MemberSince`;
CREATE TABLE IF NOT EXISTS `_MemberSince` (
  `profileId` int(10) unsigned NOT NULL default '0',
  `memberSince` datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (`profileId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_orbit_signing_status`
--

DROP TABLE IF EXISTS `_orbit_signing_status`;
CREATE TABLE IF NOT EXISTS `_orbit_signing_status` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `status` int(10) unsigned NOT NULL default '0',
  `count` int(10) unsigned NOT NULL default '1',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `status` (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `_ReportSegments`
--

DROP TABLE IF EXISTS `_ReportSegments`;
CREATE TABLE IF NOT EXISTS `_ReportSegments` (
  `accountId` int(10) unsigned NOT NULL default '0',
  `playtime` int(10) unsigned NOT NULL default '0',
  `segment` enum('low','avg','high','hardcore') default NULL,
  PRIMARY KEY  (`accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_SortedClanLadderA`
--

DROP TABLE IF EXISTS `_SortedClanLadderA`;
CREATE TABLE IF NOT EXISTS `_SortedClanLadderA` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `clanId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `clanId` (`clanId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_SortedClanLadderB`
--

DROP TABLE IF EXISTS `_SortedClanLadderB`;
CREATE TABLE IF NOT EXISTS `_SortedClanLadderB` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `clanId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `clanId` (`clanId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_SortedPlayerLadderA`
--

DROP TABLE IF EXISTS `_SortedPlayerLadderA`;
CREATE TABLE IF NOT EXISTS `_SortedPlayerLadderA` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `playerId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `playerId` (`playerId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_SortedPlayerLadderB`
--

DROP TABLE IF EXISTS `_SortedPlayerLadderB`;
CREATE TABLE IF NOT EXISTS `_SortedPlayerLadderB` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `playerId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `playerId` (`playerId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_SortedRoleairLadderA`
--

DROP TABLE IF EXISTS `_SortedRoleairLadderA`;
CREATE TABLE IF NOT EXISTS `_SortedRoleairLadderA` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `playerId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `playerId` (`playerId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_SortedRoleairLadderB`
--

DROP TABLE IF EXISTS `_SortedRoleairLadderB`;
CREATE TABLE IF NOT EXISTS `_SortedRoleairLadderB` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `playerId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `playerId` (`playerId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_SortedRolearmorLadderA`
--

DROP TABLE IF EXISTS `_SortedRolearmorLadderA`;
CREATE TABLE IF NOT EXISTS `_SortedRolearmorLadderA` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `playerId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `playerId` (`playerId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_SortedRolearmorLadderB`
--

DROP TABLE IF EXISTS `_SortedRolearmorLadderB`;
CREATE TABLE IF NOT EXISTS `_SortedRolearmorLadderB` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `playerId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `playerId` (`playerId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_SortedRoleinfantryLadderA`
--

DROP TABLE IF EXISTS `_SortedRoleinfantryLadderA`;
CREATE TABLE IF NOT EXISTS `_SortedRoleinfantryLadderA` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `playerId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `playerId` (`playerId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_SortedRoleinfantryLadderB`
--

DROP TABLE IF EXISTS `_SortedRoleinfantryLadderB`;
CREATE TABLE IF NOT EXISTS `_SortedRoleinfantryLadderB` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `playerId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `playerId` (`playerId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_SortedRolesupportLadderA`
--

DROP TABLE IF EXISTS `_SortedRolesupportLadderA`;
CREATE TABLE IF NOT EXISTS `_SortedRolesupportLadderA` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `playerId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `playerId` (`playerId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_SortedRolesupportLadderB`
--

DROP TABLE IF EXISTS `_SortedRolesupportLadderB`;
CREATE TABLE IF NOT EXISTS `_SortedRolesupportLadderB` (
  `pos` int(10) unsigned NOT NULL auto_increment,
  `playerId` int(10) unsigned NOT NULL default '0',
  `score` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`pos`),
  KEY `playerId` (`playerId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_TestReport`
--

DROP TABLE IF EXISTS `_TestReport`;
CREATE TABLE IF NOT EXISTS `_TestReport` (
  `accountId` int(10) unsigned NOT NULL default '0',
  `playtime` int(10) unsigned NOT NULL default '0',
  `segment` enum('low','avg','high','hardcore') default 'low',
  PRIMARY KEY  (`accountId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `_UserNotifications`
--

DROP TABLE IF EXISTS `_UserNotifications`;
CREATE TABLE IF NOT EXISTS `_UserNotifications` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `notificationType` int(10) unsigned NOT NULL default '0',
  `profileId` int(10) unsigned NOT NULL default '0',
  `argument` int(10) unsigned NOT NULL default '0',
  `argument_str` varchar(255) character set utf8 NOT NULL default '',
  PRIMARY KEY  (`id`),
  KEY `profileId` (`profileId`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;


-- phpMyAdmin SQL Dump
-- version 2.11.9.3
-- http://www.phpmyadmin.net
--
-- Host: mgdb07
-- Generation Time: Sep 08, 2015 at 08:55 AM
-- Server version: 4.1.22
-- PHP Version: 5.2.9

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `live`
--

-- --------------------------------------------------------

--
-- Table structure for table `BannedStrings`
--

DROP TABLE IF EXISTS `BannedStrings`;
CREATE TABLE IF NOT EXISTS `BannedStrings` (
  `isName` tinyint(1) NOT NULL default '0',
  `isChatWord` tinyint(1) NOT NULL default '0',
  `isServerName` tinyint(1) NOT NULL default '0',
  `string` varchar(64) NOT NULL default '',
  PRIMARY KEY  (`string`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Dumping data for table `BannedStrings`
--

INSERT INTO `BannedStrings` (`isName`, `isChatWord`, `isServerName`, `string`) VALUES
(1, 0, 1, 'admin'),
(1, 0, 1, 'anal'),
(1, 1, 1, 'anus'),
(1, 0, 1, 'arse'),
(1, 0, 1, 'assgate'),
(1, 1, 1, 'asshole'),
(1, 1, 1, 'beatch'),
(1, 1, 1, 'biatch'),
(1, 1, 1, 'bitch'),
(1, 0, 1, 'bollocks'),
(1, 0, 1, 'butt'),
(1, 1, 1, 'byatch'),
(1, 0, 1, 'clit'),
(1, 1, 1, 'cock'),
(1, 0, 1, 'cocksucker'),
(1, 1, 1, 'company of heroes is better than wic by far'),
(1, 1, 1, 'coon'),
(1, 0, 1, 'cum'),
(1, 1, 1, 'cunt'),
(0, 0, 1, 'developer'),
(1, 1, 1, 'dick'),
(1, 0, 1, 'faggot'),
(1, 0, 1, 'fanny'),
(1, 1, 1, 'fitta'),
(1, 0, 1, 'flange'),
(1, 1, 1, 'fuck'),
(1, 1, 1, 'goatse.cx'),
(1, 1, 1, 'heil hitler'),
(1, 0, 1, 'hitler'),
(0, 0, 1, 'massive'),
(1, 0, 1, 'moderator'),
(1, 0, 1, 'motherfucker'),
(0, 0, 1, 'msv'),
(1, 0, 1, 'mussolini'),
(1, 0, 1, 'nazi'),
(1, 1, 1, 'nigger'),
(1, 0, 1, 'penis'),
(1, 0, 1, 'piss'),
(1, 0, 1, 'plonker'),
(1, 1, 1, 'pussy'),
(0, 0, 1, 'server'),
(1, 0, 1, 'sex'),
(1, 0, 1, 'shit'),
(1, 1, 1, 'sieg heil'),
(0, 0, 1, 'sierra'),
(1, 1, 1, 'slut'),
(1, 0, 1, 'sodomites'),
(1, 0, 1, 'stalin'),
(1, 0, 1, 'tits'),
(1, 1, 1, 'tubgirl.com'),
(1, 0, 1, 'turd'),
(1, 1, 1, 'twat'),
(1, 0, 1, 'vagina'),
(1, 0, 1, 'wank'),
(1, 1, 1, 'whore'),
(0, 0, 1, 'worldinconflict');

-- --------------------------------------------------------

--
-- Table structure for table `Banners`
--

DROP TABLE IF EXISTS `Banners`;
CREATE TABLE IF NOT EXISTS `Banners` (
  `bannerId` int(11) NOT NULL auto_increment,
  `supplierId` int(10) unsigned default NULL,
  `url` varchar(255) default NULL,
  `hash` bigint(20) unsigned default NULL,
  `revoked` tinyint(1) default '0',
  `impressions` int(10) unsigned default '0',
  `type` int(3) unsigned default '0',
  PRIMARY KEY  (`bannerId`),
  KEY `hash` (`hash`),
  KEY `supplierId` (`supplierId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `Banners`
--


-- --------------------------------------------------------

--
-- Table structure for table `ClanStatsTweakables`
--

DROP TABLE IF EXISTS `ClanStatsTweakables`;
CREATE TABLE IF NOT EXISTS `ClanStatsTweakables` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `aKey` varchar(32) NOT NULL default '',
  `aValue` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=163 ;

--
-- Dumping data for table `ClanStatsTweakables`
--

INSERT INTO `ClanStatsTweakables` (`id`, `aKey`, `aValue`) VALUES
(109, 'medal_winStreak_BronzeTrigger', 3),
(110, 'medal_winStreak_SilverTrigger', 7),
(111, 'medal_winStreak_GoldTrigger', 30),
(112, 'medal_winStreak_BronzeScoreMod', 0),
(113, 'medal_winStreak_SilverScoreMod', 0),
(114, 'medal_winStreak_GoldScoreMod', 0),
(115, 'medal_domSpec_BronzeTrigger', 5),
(116, 'medal_domSpec_SilverTrigger', 50),
(117, 'medal_domSpec_GoldTrigger', 7),
(118, 'medal_domSpec_BronzeScoreMod', 0),
(119, 'medal_domSpec_SilverScoreMod', 0),
(120, 'medal_domSpec_GoldScoreMod', 0),
(121, 'medal_domEx_BronzeTrigger', 1),
(122, 'medal_domEx_SilverTrigger', 7),
(123, 'medal_domEx_GoldTrigger', 5),
(124, 'medal_domEx_BronzeScoreMod', 0),
(125, 'medal_domEx_SilverScoreMod', 0),
(126, 'medal_domEx_GoldScoreMod', 0),
(127, 'medal_assSpec_BronzeTrigger', 5),
(128, 'medal_assSpec_SilverTrigger', 50),
(129, 'medal_assSpec_GoldTrigger', 7),
(130, 'medal_assSpec_BronzeScoreMod', 0),
(131, 'medal_assSpec_SilverScoreMod', 0),
(132, 'medal_assSpec_GoldScoreMod', 0),
(133, 'medal_assEx_BronzeTrigger', 1),
(134, 'medal_assEx_SilverTrigger', 7),
(135, 'medal_assEx_GoldTrigger', 5),
(136, 'medal_assEx_BronzeScoreMod', 0),
(137, 'medal_assEx_SilverScoreMod', 0),
(138, 'medal_assEx_GoldScoreMod', 0),
(139, 'medal_towSpec_BronzeTrigger', 5),
(140, 'medal_towSpec_SilverTrigger', 50),
(141, 'medal_towSpec_GoldTrigger', 7),
(142, 'medal_towSpec_BronzeScoreMod', 0),
(143, 'medal_towSpec_SilverScoreMod', 0),
(144, 'medal_towSpec_GoldScoreMod', 0),
(145, 'medal_towEx_BronzeTrigger', 1),
(146, 'medal_towEx_SilverTrigger', 7),
(147, 'medal_towEx_GoldTrigger', 5),
(148, 'medal_towEx_BronzeScoreMod', 0),
(149, 'medal_towEx_SilverScoreMod', 0),
(150, 'medal_towEx_GoldScoreMod', 0),
(151, 'medal_hsAwd_BronzeTrigger', 25000),
(152, 'medal_hsAwd_SilverTrigger', 300000),
(153, 'medal_hsAwd_GoldTrigger', 2500000),
(154, 'medal_hsAwd_BronzeScoreMod', 0),
(155, 'medal_hsAwd_SilverScoreMod', 0),
(156, 'medal_hsAwd_GoldScoreMod', 0),
(157, 'medal_tcAwd_BronzeTrigger', 1),
(158, 'medal_tcAwd_SilverTrigger', 1),
(159, 'medal_tcAwd_GoldTrigger', 50),
(160, 'medal_tcAwd_BronzeScoreMod', 0),
(161, 'medal_tcAwd_SilverScoreMod', 0),
(162, 'medal_tcAwd_GoldScoreMod', 0);

-- --------------------------------------------------------

--
-- Table structure for table `Countries`
--

DROP TABLE IF EXISTS `Countries`;
CREATE TABLE IF NOT EXISTS `Countries` (
  `cc` varchar(4) character set utf8 NOT NULL default '',
  `cn` varchar(64) character set utf8 NOT NULL default '',
  PRIMARY KEY  (`cc`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `Countries`
--

INSERT INTO `Countries` (`cc`, `cn`) VALUES
('A1', 'Anonymous Proxy'),
('A2', 'Satellite Provider'),
('AD', 'Andorra'),
('AE', 'United Arab Emirates'),
('AF', 'Afghanistan'),
('AG', 'Antigua and Barbuda'),
('AI', 'Anguilla'),
('AL', 'Albania'),
('AM', 'Armenia'),
('AN', 'Netherlands Antilles'),
('AO', 'Angola'),
('AP', 'Asia/Pacific Region'),
('AQ', 'Antarctica'),
('AR', 'Argentina'),
('AS', 'American Samoa'),
('AT', 'Austria'),
('AU', 'Australia'),
('AW', 'Aruba'),
('AX', 'Aland Islands'),
('AZ', 'Azerbaijan'),
('BA', 'Bosnia and Herzegovina'),
('BB', 'Barbados'),
('BD', 'Bangladesh'),
('BE', 'Belgium'),
('BF', 'Burkina Faso'),
('BG', 'Bulgaria'),
('BH', 'Bahrain'),
('BI', 'Burundi'),
('BJ', 'Benin'),
('BM', 'Bermuda'),
('BN', 'Brunei Darussalam'),
('BO', 'Bolivia'),
('BR', 'Brazil'),
('BS', 'Bahamas'),
('BT', 'Bhutan'),
('BV', 'Bouvet Island'),
('BW', 'Botswana'),
('BY', 'Belarus'),
('BZ', 'Belize'),
('CA', 'Canada'),
('CC', 'Cocos Islands'),
('CD', 'Congo, The Democratic Republic of the'),
('CF', 'Central African Republic'),
('CG', 'Congo'),
('CH', 'Switzerland'),
('CI', 'Cote D''Ivoire'),
('CK', 'Cook Islands'),
('CL', 'Chile'),
('CM', 'Cameroon'),
('CN', 'China'),
('CO', 'Colombia'),
('CR', 'Costa Rica'),
('CU', 'Cuba'),
('CV', 'Cape Verde'),
('CX', 'Christmas Island'),
('CY', 'Cyprus'),
('CZ', 'Czech Republic'),
('DE', 'Germany'),
('DJ', 'Djibouti'),
('DK', 'Denmark'),
('DM', 'Dominica'),
('DO', 'Dominican Republic'),
('DZ', 'Algeria'),
('EC', 'Ecuador'),
('EE', 'Estonia'),
('EG', 'Egypt'),
('EH', 'Western Sahara'),
('ER', 'Eritrea'),
('ES', 'Spain'),
('ET', 'Ethiopia'),
('EU', 'Europe'),
('FI', 'Finland'),
('FJ', 'Fiji'),
('FK', 'Falkland Islands (Malvinas)'),
('FM', 'Micronesia, Federated States of'),
('FO', 'Faroe Islands'),
('FR', 'France'),
('GA', 'Gabon'),
('GB', 'United Kingdom'),
('GD', 'Grenada'),
('GE', 'Georgia'),
('GF', 'French Guiana'),
('GG', 'Guernsey'),
('GH', 'Ghana'),
('GI', 'Gibraltar'),
('GL', 'Greenland'),
('GM', 'Gambia'),
('GN', 'Guinea'),
('GP', 'Guadeloupe'),
('GQ', 'Equatorial Guinea'),
('GR', 'Greece'),
('GS', 'South Georgia and the South Sandwich Islands'),
('GT', 'Guatemala'),
('GU', 'Guam'),
('GW', 'Guinea-Bissau'),
('GY', 'Guyana'),
('HK', 'Hong Kong'),
('HM', 'Heard Island and McDonald Islands'),
('HN', 'Honduras'),
('HR', 'Croatia'),
('HT', 'Haiti'),
('HU', 'Hungary'),
('ID', 'Indonesia'),
('IE', 'Ireland'),
('IL', 'Israel'),
('IM', 'Isle of Man'),
('IN', 'India'),
('IO', 'British Indian Ocean Territory'),
('IQ', 'Iraq'),
('IR', 'Iran, Islamic Republic of'),
('IS', 'Iceland'),
('IT', 'Italy'),
('JE', 'lNc6RKhQVrO9c'),
('JM', 'Jamaica'),
('JO', 'Jordan'),
('JP', 'Japan'),
('KE', 'Kenya'),
('KG', 'Kyrgyzstan'),
('KH', 'Cambodia'),
('KI', 'Kiribati'),
('KM', 'Comoros'),
('KN', 'Saint Kitts and Nevis'),
('KP', 'Korea, Democratic People''s Republic of'),
('KR', 'Korea, Republic of'),
('KW', 'Kuwait'),
('KY', 'Cayman Islands'),
('KZ', 'Kazakstan'),
('LA', 'Lao People''s Democratic Republic'),
('LB', 'Lebanon'),
('LC', 'Saint Lucia'),
('LI', 'Liechtenstein'),
('LK', 'Sri Lanka'),
('LR', 'Liberia'),
('LS', 'Lesotho'),
('LT', 'Lithuania'),
('LU', 'Luxembourg'),
('LV', 'Latvia'),
('LY', 'Libyan Arab Jamahiriya'),
('MA', 'Morocco'),
('MC', 'Monaco'),
('MD', 'Moldova, Republic of'),
('ME', 'Montenegro'),
('MG', 'Madagascar'),
('MH', 'Marshall Islands'),
('MK', 'Macedonia'),
('ML', 'Mali'),
('MM', 'Myanmar'),
('MN', 'Mongolia'),
('MO', 'Macau'),
('MP', 'Northern Mariana Islands'),
('MQ', 'Martinique'),
('MR', 'Mauritania'),
('MS', 'Montserrat'),
('MT', 'Malta'),
('MU', 'Mauritius'),
('MV', 'Maldives'),
('MW', 'Malawi'),
('MX', 'Mexico'),
('MY', 'Malaysia'),
('MZ', 'Mozambique'),
('NA', 'Namibia'),
('NC', 'New Caledonia'),
('NE', 'Niger'),
('NF', 'Norfolk Island'),
('NG', 'Nigeria'),
('NI', 'Nicaragua'),
('NL', 'Netherlands'),
('NO', 'Norway'),
('NP', 'Nepal'),
('NR', 'Nauru'),
('NU', 'Niue'),
('NZ', 'New Zealand'),
('OM', 'Oman'),
('PA', 'Panama'),
('PE', 'Peru'),
('PF', 'French Polynesia'),
('PG', 'Papua New Guinea'),
('PH', 'Philippines'),
('PK', 'Pakistan'),
('PL', 'Poland'),
('PM', 'Saint Pierre and Miquelon'),
('PN', 'Pitcairn'),
('PR', 'Puerto Rico'),
('PS', 'Palestinian Territory, Occupied'),
('PT', 'Portugal'),
('PW', 'Palau'),
('PY', 'Paraguay'),
('QA', 'Qatar'),
('RE', 'Reunion'),
('RO', 'Romania'),
('RS', 'Serbia'),
('RU', 'Russian Federation'),
('RW', 'Rwanda'),
('SA', 'Saudi Arabia'),
('SB', 'Solomon Islands'),
('SC', 'Seychelles'),
('SD', 'Sudan'),
('SE', 'Sweden'),
('SG', 'Singapore'),
('SH', 'Saint Helena'),
('SI', 'Slovenia'),
('SJ', 'Svalbard and Jan Mayen'),
('SK', 'Slovakia'),
('SL', 'Sierra Leone'),
('SM', 'San Marino'),
('SN', 'Senegal'),
('SO', 'Somalia'),
('SR', 'Suriname'),
('ST', 'Sao Tome and Principe'),
('SV', 'El Salvador'),
('SY', 'Syrian Arab Republic'),
('SZ', 'Swaziland'),
('TC', 'Turks and Caicos Islands'),
('TD', 'Chad'),
('TF', 'French Southern Territories'),
('TG', 'Togo'),
('TH', 'Thailand'),
('TJ', 'Tajikistan'),
('TK', 'Tokelau'),
('TL', 'Timor-Leste'),
('TM', 'Turkmenistan'),
('TN', 'Tunisia'),
('TO', 'Tonga'),
('TR', 'Turkey'),
('TT', 'Trinidad and Tobago'),
('TV', 'Tuvalu'),
('TW', 'Taiwan'),
('TZ', 'Tanzania, United Republic of'),
('UA', 'Ukraine'),
('UG', 'Uganda'),
('UM', 'United States Minor Outlying Islands'),
('US', 'United States'),
('UY', 'Uruguay'),
('UZ', 'Uzbekistan'),
('VA', 'Holy See (Vatican City State)'),
('VC', 'Saint Vincent and the Grenadines'),
('VE', 'Venezuela'),
('VG', 'Virgin Islands, British'),
('VI', 'Virgin Islands, U.S.'),
('VN', 'Vietnam'),
('VU', 'Vanuatu'),
('WF', 'Wallis and Futuna'),
('WS', 'Samoa'),
('YE', 'Yemen'),
('YT', 'Mayotte'),
('ZA', 'South Africa'),
('ZM', 'Zambia'),
('ZW', 'Zimbabwe');

-- --------------------------------------------------------

--
-- Table structure for table `Distributions`
--

DROP TABLE IF EXISTS `Distributions`;
CREATE TABLE IF NOT EXISTS `Distributions` (
  `distributionid` int(11) unsigned NOT NULL auto_increment,
  `productid` int(11) unsigned NOT NULL default '0',
  `distribution` varchar(32) NOT NULL default '',
  `latestversion` varchar(16) NOT NULL default '',
  PRIMARY KEY  (`distributionid`,`productid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=3 ;

--
-- Dumping data for table `Distributions`
--

INSERT INTO `Distributions` (`distributionid`, `productid`, `distribution`, `latestversion`) VALUES
(1, 1, 'pre-greenlight', '1.0.0.8'),
(2, 2, 'English', '0.0.9-pre-beta');

-- --------------------------------------------------------

--
-- Table structure for table `GeoIP`
--

DROP TABLE IF EXISTS `GeoIP`;
CREATE TABLE IF NOT EXISTS `GeoIP` (
  `start_ip` varchar(15) NOT NULL default '',
  `end_ip` varchar(15) NOT NULL default '',
  `start` int(10) unsigned NOT NULL default '0',
  `end` int(10) unsigned NOT NULL default '0',
  `cc` char(2) NOT NULL default '',
  `cn` varchar(50) NOT NULL default '',
  UNIQUE KEY `start` (`start`),
  KEY `end` (`end`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `GeoIP`
--

INSERT INTO `GeoIP` (`start_ip`, `end_ip`, `start`, `end`, `cc`, `cn`) VALUES
('2.6.190.56', '2.6.190.63', 33996344, 33996351, 'US', 'United States'),
('3.0.0.0', '4.17.135.31', 50331648, 68257567, 'US', 'United States'),
('4.17.135.32', '222.255.255.255', 68257568, 3741319167, 'US', 'United States');

-- --------------------------------------------------------

--
-- Table structure for table `MapHashes`
--

DROP TABLE IF EXISTS `MapHashes`;
CREATE TABLE IF NOT EXISTS `MapHashes` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `hash` bigint(20) unsigned NOT NULL default '0',
  `name` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `hash` (`hash`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `MapHashes`
--


-- --------------------------------------------------------

--
-- Table structure for table `OptionalContent`
--

DROP TABLE IF EXISTS `OptionalContent`;
CREATE TABLE IF NOT EXISTS `OptionalContent` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `hash` int(10) unsigned NOT NULL default '0',
  `name` varchar(255) NOT NULL default '',
  `url` varchar(255) NOT NULL default '',
  `groupMembership` int(10) unsigned NOT NULL default '0',
  `country` varchar(2) NOT NULL default '',
  `continent` varchar(2) NOT NULL default '',
  `lang` varchar(2) NOT NULL default '',
  `n_retries` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=637 ;

--
-- Dumping data for table `OptionalContent`
--

INSERT INTO `OptionalContent` (`id`, `hash`, `name`, `url`, `groupMembership`, `country`, `continent`, `lang`, `n_retries`) VALUES
(169, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'SE', 'EU', 'EN', 864),
(171, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'SE', 'EU', 'GB', 864),
(173, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'CZ', 864),
(175, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'D2', 864),
(177, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'DE', 864),
(179, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'ES', 864),
(181, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'FR', 864),
(183, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'HU', 864),
(185, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'IT', 864),
(187, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'PL', 864),
(189, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'RU', 864),
(191, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'ZH', 864),
(193, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'T1', 864),
(195, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'T2', 864),
(197, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'JP', 864),
(199, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'S1', 864),
(201, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'S2', 864),
(203, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'S3', 864),
(205, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'TW', 864),
(222, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'EN', 0),
(224, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'GB', 0),
(226, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'CZ', 0),
(228, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'D2', 0),
(230, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'DE', 0),
(232, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'ES', 0),
(234, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'FR', 0),
(236, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'HU', 0),
(238, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'IT', 0),
(240, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'PL', 0),
(242, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'RU', 0),
(244, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'ZH', 0),
(246, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'T1', 0),
(248, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'T2', 0),
(250, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'JP', 0),
(252, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'S1', 0),
(254, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'S2', 0),
(256, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'S3', 0),
(259, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'CZ', 864),
(260, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'D2', 864),
(261, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'DE', 864),
(262, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'EN', 864),
(263, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'ES', 864),
(264, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'FR', 864),
(265, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'GB', 864),
(266, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'HU', 864),
(267, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'IT', 864),
(268, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'JP', 864),
(269, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'PL', 864),
(270, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'RU', 864),
(271, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'S1', 864),
(272, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'S2', 864),
(273, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'S3', 864),
(274, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'CZ', 0),
(275, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'D2', 0),
(276, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'DE', 0),
(277, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'EN', 0),
(278, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'ES', 0),
(279, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'FR', 0),
(280, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'GB', 0),
(281, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'HU', 0),
(282, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'IT', 0),
(283, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'JP', 0),
(284, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'PL', 0),
(285, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'RU', 0),
(286, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'S1', 0),
(287, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'S2', 0),
(288, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'S3', 0),
(304, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'CZ', 864),
(305, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'D2', 864),
(306, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'DE', 864),
(307, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'EN', 864),
(308, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'ES', 864),
(309, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'FR', 864),
(310, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'GB', 864),
(311, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'HU', 864),
(312, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'IT', 864),
(313, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'JP', 864),
(314, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'PL', 864),
(315, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'RU', 864),
(316, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'S1', 864),
(317, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'S2', 864),
(318, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'S3', 864),
(334, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'CZ', 0),
(335, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'D2', 0),
(336, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'DE', 0),
(337, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'EN', 0),
(338, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'ES', 0),
(339, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'FR', 0),
(340, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'GB', 0),
(341, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'HU', 0),
(342, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'IT', 0),
(343, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'JP', 0),
(344, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'PL', 0),
(345, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'RU', 0),
(346, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'S1', 0),
(347, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'S2', 0),
(348, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'S3', 0),
(379, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'CZ', 0),
(380, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'D2', 0),
(381, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'DE', 0),
(382, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'EN', 0),
(383, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'ES', 0),
(384, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'FR', 0),
(385, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'GB', 0),
(386, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'HU', 0),
(387, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'IT', 0),
(388, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'JP', 0),
(389, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'PL', 0),
(390, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'RU', 0),
(391, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'S1', 0),
(392, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'S2', 0),
(393, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'S3', 0),
(409, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'CZ', 864),
(410, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'D2', 864),
(411, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'DE', 864),
(412, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'EN', 864),
(413, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'ES', 864),
(414, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'FR', 864),
(415, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'GB', 864),
(416, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'HU', 864),
(417, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'IT', 864),
(418, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'JP', 864),
(419, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'PL', 864),
(420, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'RU', 864),
(421, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'S1', 864),
(422, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'S2', 864),
(423, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'S3', 864),
(424, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'CZ', 864),
(425, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'D2', 864),
(426, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'DE', 864),
(427, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'EN', 864),
(428, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'ES', 864),
(429, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'FR', 864),
(430, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'GB', 864),
(431, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'HU', 864),
(432, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'IT', 864),
(433, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'JP', 864),
(434, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'PL', 864),
(435, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'RU', 864),
(436, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'S1', 864),
(437, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'S2', 864),
(438, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'S3', 864),
(439, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'V0', 864),
(440, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'V0', 0),
(443, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'V0', 864),
(444, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'V0', 0),
(446, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'V0', 864),
(447, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'V0', 0),
(452, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'SE', 'EU', 'V0', 864),
(454, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'V0', 864),
(455, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'V0', 0),
(457, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'V1', 864),
(458, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'V1', 0),
(461, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'V1', 864),
(462, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'V1', 0),
(464, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'V1', 864),
(465, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'V1', 0),
(470, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'V1', 864),
(472, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'V1', 864),
(473, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'V1', 0),
(475, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'V2', 864),
(476, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'V2', 0),
(479, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'V2', 864),
(480, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'V2', 0),
(482, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'V2', 864),
(483, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'V2', 0),
(488, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'V2', 864),
(490, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'V2', 864),
(491, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'V2', 0),
(493, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'V4', 864),
(494, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'V4', 0),
(497, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'V4', 864),
(498, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'V4', 0),
(500, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'V4', 864),
(501, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'V4', 0),
(505, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'V4', 864),
(507, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'V4', 864),
(508, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'V4', 0),
(510, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'V5', 864),
(511, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'V5', 0),
(514, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'V5', 864),
(515, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'V5', 0),
(517, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'V5', 864),
(518, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'V5', 0),
(523, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'V5', 864),
(525, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'V5', 864),
(526, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'V5', 0),
(528, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'V6', 864),
(529, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'V6', 0),
(532, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'V6', 864),
(533, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'V6', 0),
(535, 4, 'do_Valley', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Valley.sdf', 0, 'DE', 'EU', 'V6', 864),
(536, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'V6', 0),
(539, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'V6', 864),
(543, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'V6', 864),
(544, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'V6', 0),
(546, 6, 'as_Ozzault', 'http://download1.wic.ngz-server.de/wic/maps/en/as_Ozzault.sdf', 0, 'DE', 'EU', 'V3', 864),
(547, 6, 'as_Ozzault', 'http://redirect.multiplay.co.uk/downloads/wic/maps/as_Ozzault.sdf', 0, 'UK', 'EU', 'V3', 0),
(550, 7, 'do_Paradise', 'http://download1.wic.ngz-server.de/wic/maps/en/do_Paradise.sdf', 0, 'DE', 'EU', 'V3', 864),
(551, 7, 'do_Paradise', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Paradise.sdf', 0, 'UK', 'EU', 'V3', 0),
(553, 4, 'do_Valley', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Valley.sdf', 0, 'UK', 'EU', 'V3', 0),
(558, 3, 'do_Virginia', 'http://download1.wic.ngz-server.de/wic/maps/en/virginia.sdf', 0, 'DE', 'EU', 'V3', 864),
(560, 5, 'tw_Arizona', 'http://download1.wic.ngz-server.de/wic/maps/en/tw_Arizona.sdf', 0, 'DE', 'EU', 'V3', 864),
(561, 5, 'tw_Arizona', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_Arizona.sdf', 0, 'UK', 'EU', 'V3', 0),
(563, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'CZ', 0),
(564, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'D2', 0),
(565, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'DE', 0),
(566, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'EN', 0),
(567, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'ES', 0),
(568, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'FR', 0),
(569, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'GB', 0),
(570, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'HU', 0),
(571, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'IT', 0),
(572, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'JP', 0),
(573, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'PL', 0),
(574, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'RU', 0),
(575, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'S1', 0),
(576, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'S2', 0),
(577, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'S3', 0),
(578, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'CZ', 0),
(579, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'D2', 0),
(580, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'DE', 0),
(581, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'EN', 0),
(582, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'ES', 0),
(583, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'FR', 0),
(584, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'GB', 0),
(585, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'HU', 0),
(586, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'IT', 0),
(587, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'JP', 0),
(588, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'PL', 0),
(589, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'RU', 0),
(590, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'S1', 0),
(591, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'S2', 0),
(592, 8, 'do_Airport', 'http://redirect.multiplay.co.uk/downloads/wic/maps/do_Airport.sdf', 0, 'UK', 'EU', 'S3', 0),
(593, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'V0', 0),
(594, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'V1', 0),
(595, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'V2', 0),
(596, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'V3', 0),
(597, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'V4', 0),
(598, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'CZ', 0),
(599, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'D2', 0),
(600, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'DE', 0),
(601, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'EN', 0),
(602, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'ES', 0),
(603, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'FR', 0),
(604, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'GB', 0),
(605, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'HU', 0),
(606, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'IT', 0),
(607, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'JP', 0),
(608, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'PL', 0),
(609, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'RU', 0),
(610, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'S1', 0),
(611, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'S2', 0),
(612, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'S3', 0),
(613, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'CZ', 0),
(614, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'D2', 0),
(615, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'DE', 0),
(616, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'EN', 0),
(617, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'ES', 0),
(618, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'FR', 0),
(619, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'GB', 0),
(620, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'HU', 0),
(621, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'IT', 0),
(622, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'JP', 0),
(623, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'PL', 0),
(624, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'RU', 0),
(625, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'S1', 0),
(626, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'S2', 0),
(627, 9, 'tw_Bocage', 'http://redirect.multiplay.co.uk/downloads/wic/maps/tw_bocage.sdf', 0, 'UK', 'EU', 'S3', 0),
(628, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'V0', 0),
(629, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'V1', 0),
(630, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'V2', 0),
(631, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'V3', 0),
(632, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'V4', 0),
(633, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'V5', 0),
(634, 8, 'do_Airport', 'http://static5.ubi.com/u/massive/World in Conflict/do_Airport.sdf', 0, 'SE', 'EU', 'V6', 0),
(635, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'V5', 0),
(636, 9, 'tw_Bocage', 'http://static5.ubi.com/u/massive/World in Conflict/tw_bocage.sdf', 0, 'SE', 'EU', 'V6', 0);

-- --------------------------------------------------------

--
-- Table structure for table `PlayerStatsTweakables`
--

DROP TABLE IF EXISTS `PlayerStatsTweakables`;
CREATE TABLE IF NOT EXISTS `PlayerStatsTweakables` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `aKey` varchar(32) default NULL,
  `aValue` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=249 ;

--
-- Dumping data for table `PlayerStatsTweakables`
--

INSERT INTO `PlayerStatsTweakables` (`id`, `aKey`, `aValue`) VALUES
(56, 'medal_infAch_NumForBronze', 1),
(57, 'medal_infAch_NumForSilver', 10),
(58, 'medal_infAch_StreakForGold', 10),
(59, 'medal_infAch_BronzeScoreMod', 0),
(60, 'medal_infAch_SilverScoreMod', 0),
(61, 'medal_infAch_GoldScoreMod', 0),
(62, 'medal_supAch_NumForBronze', 1),
(63, 'medal_supAch_NumForSilver', 10),
(64, 'medal_supAch_StreakForGold', 10),
(65, 'medal_supAch_BronzeScoreMod', 0),
(66, 'medal_supAch_SilverScoreMod', 0),
(67, 'medal_supAch_GoldScoreMod', 0),
(68, 'medal_armAch_NumForBronze', 1),
(69, 'medal_armAch_NumForSilver', 10),
(70, 'medal_armAch_StreakForGold', 10),
(71, 'medal_armAch_BronzeScoreMod', 0),
(72, 'medal_armAch_SilverScoreMod', 0),
(73, 'medal_armAch_GoldScoreMod', 0),
(74, 'medal_airAch_NumForBronze', 1),
(75, 'medal_airAch_NumForSilver', 10),
(76, 'medal_airAch_StreakForGold', 10),
(77, 'medal_airAch_BronzeScoreMod', 0),
(78, 'medal_airAch_SilverScoreMod', 0),
(79, 'medal_airAch_GoldScoreMod', 0),
(80, 'medal_scoreAch_NumForBronze', 1),
(81, 'medal_scoreAch_NumForSilver', 10),
(82, 'medal_scoreAch_StreakForGold', 10),
(83, 'medal_scoreAch_BronzeScoreMod', 0),
(84, 'medal_scoreAch_SilverScoreMod', 0),
(85, 'medal_scoreAch_GoldScoreMod', 0),
(86, 'medal_taAch_NumForBronze', 25),
(87, 'medal_taAch_NumForSilver', 125),
(88, 'medal_taAch_NumForGold', 15),
(89, 'medal_taAch_BronzeScoreMod', 0),
(90, 'medal_taAch_SilverScoreMod', 0),
(91, 'medal_taAch_GoldScoreMod', 0),
(92, 'medal_infComEx_ScoreForBronze', 500),
(93, 'medal_infComEx_ScoreForSilver', 1200),
(94, 'medal_infComEx_ScoreForGold', 2000),
(95, 'medal_infComEx_BronzeScoreMod', 0),
(96, 'medal_infComEx_SilverScoreMod', 0),
(97, 'medal_infComEx_GoldScoreMod', 0),
(98, 'medal_supComEx_ScoreForBronze', 500),
(99, 'medal_supComEx_ScoreForSilver', 1200),
(100, 'medal_supComEx_ScoreForGold', 2000),
(101, 'medal_supComEx_BronzeScoreMod', 0),
(102, 'medal_supComEx_SilverScoreMod', 0),
(103, 'medal_supComEx_GoldScoreMod', 0),
(104, 'medal_armComEx_ScoreForBronze', 500),
(105, 'medal_armComEx_ScoreForSilver', 1200),
(106, 'medal_armComEx_ScoreForGold', 2000),
(107, 'medal_armComEx_BronzeScoreMod', 0),
(108, 'medal_armComEx_SilverScoreMod', 0),
(109, 'medal_armComEx_GoldScoreMod', 0),
(110, 'medal_airComEx_ScoreForBronze', 500),
(111, 'medal_airComEx_ScoreForSilver', 1200),
(112, 'medal_airComEx_ScoreForGold', 2000),
(113, 'medal_airComEx_BronzeScoreMod', 0),
(114, 'medal_airComEx_SilverScoreMod', 0),
(115, 'medal_airComEx_GoldScoreMod', 0),
(116, 'medal_winStreak_NumForBronze', 5),
(117, 'medal_winStreak_NumForSilver', 10),
(118, 'medal_winStreak_NumForGold', 20),
(119, 'medal_winStreak_BronzeScoreMod', 0),
(120, 'medal_winStreak_SilverScoreMod', 0),
(121, 'medal_winStreak_GoldScoreMod', 0),
(122, 'medal_taSpec_NumForBronze', 25),
(123, 'medal_taSpec_NumForSilver', 125),
(124, 'medal_taSpec_NumForGold', 15),
(125, 'medal_taSpec_BronzeScoreMod', 0),
(126, 'medal_taSpec_SilverScoreMod', 0),
(127, 'medal_taSpec_GoldScoreMod', 0),
(128, 'medal_domSpec_NumForBronze', 10),
(129, 'medal_domSpec_NumForSilver', 100),
(130, 'medal_domSpec_StreakForGold', 10),
(131, 'medal_domSpec_BronzeScoreMod', 0),
(132, 'medal_domSpec_SilverScoreMod', 0),
(133, 'medal_domSpec_GoldScoreMod', 0),
(134, 'medal_domEx_NumForBronze', 1),
(135, 'medal_domEx_NumForSilver', 10),
(136, 'medal_domEx_StreakForGold', 5),
(137, 'medal_domEx_BronzeScoreMod', 0),
(138, 'medal_domEx_SilverScoreMod', 0),
(139, 'medal_domEx_GoldScoreMod', 0),
(140, 'medal_assSpec_NumForBronze', 10),
(141, 'medal_assSpec_NumForSilver', 100),
(142, 'medal_assSpec_StreakForGold', 10),
(143, 'medal_assSpec_BronzeScoreMod', 0),
(144, 'medal_assSpec_SilverScoreMod', 0),
(145, 'medal_assSpec_GoldScoreMod', 0),
(146, 'medal_assEx_NumForBronze', 1),
(147, 'medal_assEx_NumForSilver', 10),
(148, 'medal_assEx_StreakForGold', 5),
(149, 'medal_assEx_BronzeScoreMod', 0),
(150, 'medal_assEx_SilverScoreMod', 0),
(151, 'medal_assEx_GoldScoreMod', 0),
(152, 'medal_towSpec_NumForBronze', 10),
(153, 'medal_towSpec_NumForSilver', 100),
(154, 'medal_towSpec_StreakForGold', 10),
(155, 'medal_towSpec_BronzeScoreMod', 0),
(156, 'medal_towSpec_SilverScoreMod', 0),
(157, 'medal_towSpec_GoldScoreMod', 0),
(158, 'medal_towEx_NumForBronze', 1),
(159, 'medal_towEx_NumForSilver', 10),
(160, 'medal_towEx_StreakForGold', 5),
(161, 'medal_towEx_BronzeScoreMod', 0),
(162, 'medal_towEx_SilverScoreMod', 0),
(163, 'medal_towEx_GoldScoreMod', 0),
(164, 'badge_infSp_ScoreForBronze', 8000),
(165, 'badge_infSp_ScoreForSilver', 50000),
(166, 'badge_infSp_ScoreForGold', 400000),
(167, 'badge_infSp_BronzeScoreMod', 0),
(168, 'badge_infSp_SilverScoreMod', 0),
(169, 'badge_infSp_GoldScoreMod', 0),
(170, 'badge_airSp_ScoreForBronze', 8000),
(171, 'badge_airSp_ScoreForSilver', 50000),
(172, 'badge_airSp_ScoreForGold', 400000),
(173, 'badge_airSp_BronzeScoreMod', 0),
(174, 'badge_airSp_SilverScoreMod', 0),
(175, 'badge_airSp_GoldScoreMod', 0),
(176, 'badge_armSp_ScoreForBronze', 8000),
(177, 'badge_armSp_ScoreForSilver', 50000),
(178, 'badge_armSp_ScoreForGold', 400000),
(179, 'badge_armSp_BronzeScoreMod', 0),
(180, 'badge_armSp_SilverScoreMod', 0),
(181, 'badge_armSp_GoldScoreMod', 0),
(182, 'badge_supSp_ScoreForBronze', 8000),
(183, 'badge_supSp_ScoreForSilver', 50000),
(184, 'badge_supSp_ScoreForGold', 400000),
(185, 'badge_supSp_BronzeScoreMod', 0),
(186, 'badge_supSp_SilverScoreMod', 0),
(187, 'badge_supSp_GoldScoreMod', 0),
(188, 'badge_scoreAch_ScoreForBronze', 10000),
(189, 'badge_scoreAch_ScoreForSilver', 100000),
(190, 'badge_scoreAch_ScoreForGold', 800000),
(191, 'badge_scoreAch_BronzeScoreMod', 0),
(192, 'badge_scoreAch_SilverScoreMod', 0),
(193, 'badge_scoreAch_GoldScoreMod', 0),
(194, 'badge_cpAch_NumForBronze', 25),
(195, 'badge_cpAch_NumForSilver', 250),
(196, 'badge_cpAch_NumForGold', 1200),
(197, 'badge_cpAch_BronzeScoreMod', 0),
(198, 'badge_cpAch_SilverScoreMod', 0),
(199, 'badge_cpAch_GoldScoreMod', 0),
(200, 'badge_fortAch_NumForBronze', 500),
(201, 'badge_fortAch_NumForSilver', 3500),
(202, 'badge_fortAch_NumForGold', 15000),
(203, 'badge_fortAch_BronzeScoreMod', 0),
(204, 'badge_fortAch_SilverScoreMod', 0),
(205, 'badge_fortAch_GoldScoreMod', 0),
(206, 'badge_mgAch_TimeForBronze', 2592000),
(207, 'badge_mgAch_TimeForSilver', 7776000),
(208, 'badge_mgAch_TimeForGold', 31536000),
(209, 'badge_mgAch_BronzeScoreMod', 0),
(210, 'badge_mgAch_SilverScoreMod', 0),
(211, 'badge_mgAch_GoldScoreMod', 0),
(212, 'badge_matchAch_NumForBronze', 10),
(213, 'badge_matchAch_NumForSilver', 200),
(214, 'badge_matchAch_NumForGold', 1500),
(215, 'badge_matchAch_BronzeScoreMod', 0),
(216, 'badge_matchAch_SilverScoreMod', 0),
(217, 'badge_matchAch_GoldScoreMod', 0),
(218, 'badge_USAAch_TimeForBronze', 3600),
(219, 'badge_USAAch_TimeForSilver', 86400),
(220, 'badge_USAAch_TimeForGold', 360000),
(221, 'badge_USAAch_BronzeScoreMod', 0),
(222, 'badge_USAAch_SilverScoreMod', 0),
(223, 'badge_USAAch_GoldScoreMod', 0),
(224, 'badge_USSRAch_TimeForBronze', 3600),
(225, 'badge_USSRAch_TimeForSilver', 86400),
(226, 'badge_USSRAch_TimeForGold', 360000),
(227, 'badge_USSRAch_BronzeScoreMod', 0),
(228, 'badge_USSRAch_SilverScoreMod', 0),
(229, 'badge_USSRAch_GoldScoreMod', 0),
(230, 'badge_NATOAch_TimeForBronze', 3600),
(231, 'badge_NATOAch_TimeForSilver', 86400),
(232, 'badge_NATOAch_TimeForGold', 360000),
(233, 'badge_NATOAch_BronzeScoreMod', 0),
(234, 'badge_NATOAch_SilverScoreMod', 0),
(235, 'badge_NATOAch_GoldScoreMod', 0),
(236, 'badge_preOrdAch_TimeForBronze', 1),
(237, 'badge_preOrdAch_TimeForSilver', 5),
(238, 'badge_preOrdAch_TimeForGold', 10),
(239, 'badge_preOrdAch_BronzeScoreMod', 0),
(240, 'badge_preOrdAch_SilverScoreMod', 0),
(241, 'badge_preOrdAch_GoldScoreMod', 0),
(242, 'medal_nukeSpec_NumForBronze', 1),
(243, 'medal_nukeSpec_NumForSilver', 10),
(244, 'medal_nukeSpec_NumForStreak', 0),
(245, 'medal_nukeSpec_StreakForGold', 8),
(246, 'medal_nukeSpec_BronzeScoreMod', 0),
(247, 'medal_nukeSpec_SilverScoreMod', 0),
(248, 'medal_nukeSpec_GoldScoreMod', 0);

-- --------------------------------------------------------

--
-- Table structure for table `Products`
--

DROP TABLE IF EXISTS `Products`;
CREATE TABLE IF NOT EXISTS `Products` (
  `productid` smallint(5) unsigned NOT NULL auto_increment,
  `productname` varchar(64) NOT NULL default '',
  UNIQUE KEY `productid` (`productid`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=3 ;

--
-- Dumping data for table `Products`
--

INSERT INTO `Products` (`productid`, `productname`) VALUES
(1, 'World in Conflict'),
(2, 'Project Eagle');

-- --------------------------------------------------------

--
-- Table structure for table `Settings`
--

DROP TABLE IF EXISTS `Settings`;
CREATE TABLE IF NOT EXISTS `Settings` (
  `aVariable` varchar(64) NOT NULL default '',
  `aValue` varchar(255) NOT NULL default '',
  `lastUpdated` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  UNIQUE KEY `aVariable` (`aVariable`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

--
-- Dumping data for table `Settings`
--

INSERT INTO `Settings` (`aVariable`, `aValue`, `lastUpdated`) VALUES
('AccountDatabaseRead', 'naaaah.massgate.net', '2006-09-26 13:35:00'),
('AccountDatabaseWrite', 'ohnoooo.massgate.net', '2006-09-26 13:35:00'),
('AllowNATNegDataRelaying', 'yes', '2007-01-25 12:05:17'),
('BannerRotationTimeMinutes', '5', '2007-02-07 06:54:20'),
('BestOfLadderNumDays', '21', '2007-12-11 07:08:33'),
('BestOfLadderNumGames', '20', '2007-09-24 08:20:01'),
('CanPlayClanMatchAgainInDays', '2', '2008-12-17 10:41:46'),
('CdkeyDatabaseRead', 'byok.massgate.net', '2006-09-26 13:35:00'),
('CdkeyDatabaseWrite', 'diy.massgate.net', '2006-09-26 13:35:00'),
('CurrentProtocolVersion', '1', '2007-05-29 16:17:27'),
('DebugUploadHost', 'FTP.asdfasdf.com', '2007-03-05 16:42:32'),
('DebugUploadHostPort', '21', '2007-03-05 16:42:32'),
('DebugUploadLogin', 'yeah', '2007-03-05 16:42:32'),
('DebugUploadPassword', 'password123', '2007-03-05 16:42:32'),
('DisplayRecruitmentQuestion', '1', '2008-02-19 09:58:53'),
('EnableAntiClanSmurfing', '0', '2010-08-26 08:36:32'),
('LastBannedStringsUpdate', '1', '2006-12-31 23:00:00'),
('LauncherCdKeyValidationEnabled', '1', '2009-03-11 09:51:58'),
('MaintenanceImminent', '0', '2009-06-22 12:50:54'),
('MasterChangelogUrl', 'http://www.massgate.net/patches/wic/update_notes_11/LANGCODE/update_notes.txt', '2009-06-22 13:00:14'),
('MasterPatchListUrl', 'http://www.massgate.net/patches/wic/latest.txt', '2007-10-08 06:59:03'),
('MaxNumberOfDaysBeforeAccountActivation', '7', '2006-09-26 13:35:00'),
('NextServerSecret', '7483884058120487025', '2007-06-20 05:59:39'),
('pcr', '100', '2009-03-11 09:49:53'),
('PlayerWinningScoreMultiplier', '1.5', '2007-10-08 06:57:09'),
('PreviousServerSecret', '17836381643814593044', '2007-06-20 05:59:39'),
('RestrictPatchByIp', '0', '2007-09-25 05:36:03'),
('ServerSecret', '9764274907678504763', '2007-06-20 05:59:39'),
('TrackMemoryAllocations', '0', '2008-09-23 06:46:41'),
('TrackSQLQueries', '0', '2009-03-10 09:17:07');

-- --------------------------------------------------------

--
-- Table structure for table `WICRankDefinitions`
--

DROP TABLE IF EXISTS `WICRankDefinitions`;
CREATE TABLE IF NOT EXISTS `WICRankDefinitions` (
  `id` int(10) unsigned NOT NULL default '0',
  `totalScore` int(10) unsigned NOT NULL default '0',
  `ladderPercentage` int(11) NOT NULL default '0',
  `db_description` varchar(32) NOT NULL default '',
  UNIQUE KEY `id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `WICRankDefinitions`
--

INSERT INTO `WICRankDefinitions` (`id`, `totalScore`, `ladderPercentage`, `db_description`) VALUES
(0, 0, 0, 'Recruit'),
(1, 600, 0, 'Private'),
(2, 2000, 0, 'Private First Class'),
(3, 5000, 0, 'Corporal'),
(4, 9000, 0, 'Sergeant'),
(5, 15000, 0, 'Staff Sergeant'),
(6, 25000, 0, 'Sergeant First Class'),
(7, 40000, 0, 'First Sergeant'),
(8, 58000, 0, 'Sergeant Major'),
(9, 77000, 0, '2nd Lieutenant'),
(10, 110000, 33, '1st Lieutenant'),
(11, 145000, 55, 'Captain'),
(12, 180000, 70, 'Major'),
(13, 215000, 80, 'Lieutenant Colonel'),
(14, 260000, 87, 'Colonel'),
(15, 310000, 91, 'Brigadier General'),
(16, 365000, 95, 'Major General'),
(17, 430000, 98, 'Lieutenant General'),
(18, 500000, 99, 'General');
