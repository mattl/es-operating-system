/*
 * Copyright 2008 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NINTENDO_ES_KERNEL_ATA_H_INCLUDED
#define NINTENDO_ES_KERNEL_ATA_H_INCLUDED

#include <es/types.h>

namespace ATAttachment
{

    namespace Register
    {
        // Command block register (CS1 = N, CS0 = A)
        static const u8 DATA = 0;
        static const u8 ERROR = 1;              // read only
        static const u8 FEATURES = 1;           // write only
        static const u8 SECTOR_COUNT = 2;
        static const u8 LBA_LOW = 3;
        static const u8 LBA_MID = 4;
        static const u8 LBA_HIGH = 5;
        static const u8 DEVICE = 6;
        static const u8 STATUS = 7;             // read only
        static const u8 COMMAND = 7;            // write only

        // PACKET command
        static const u8 INTERRUPT_REASON = 2;
        static const u8 BYTE_COUNT_LOW = 4;
        static const u8 BYTE_COUNT_HIGH = 5;
        static const u8 DEVICE_SELECT = 6;

        // Control block register (CS1 = A; CS0 = N)
        static const u8 ALTERNATE_STATUS = 6;   // read only
        static const u8 DEVICE_CONTROL = 6;     // write only
    };

    namespace Device
    {
        static const u8 DEV = 0x10;
        static const u8 LBA = 0x40;

        static const u8 MASTER = 0;
        static const u8 SLAVE = DEV;
    };

    namespace DeviceControl
    {
        static const u8 NIEN = 0x02;
        static const u8 SRST = 0x04;
        static const u8 HOB = 0x80;
    };

    namespace Error
    {
        static const u8 ABRT = 0x04;
    };

    namespace Features
    {
        static const u8 DMA = 0x01;
        static const u8 OVL = 0x02;
    };

    namespace InterruptReason
    {
        static const u8 CD = 0x01;
        static const u8 IO = 0x02;
        static const u8 REL = 0x04;
    };

    namespace Status
    {
        static const u8 ERR = 0x01;
        static const u8 CHK = 0x01;
        static const u8 DRQ = 0x08;
        static const u8 DF = 0x20;
        static const u8 SE = 0x20;
        static const u8 DRDY = 0x40;
        static const u8 BSY = 0x80;
    };

    namespace Command
    {
        static const u8 CFA_ERASE_SECTORS = 0xc0;
        static const u8 CFA_REQUEST_EXTENDED_ERROR_CODE = 0x03;
        static const u8 CFA_TRANSLATE_SECTOR = 0x87;
        static const u8 CFA_WRITE_MULTIPLE_WITHOUT_ERASE = 0xcd;
        static const u8 CFA_WRITE_SECTORS_WITHOUT_ERASE = 0x38;
        static const u8 CHECK_MEDIA_CARD_TYPE = 0xd1;
        static const u8 CHECK_POWER_MODE = 0xe5;
        static const u8 DEVICE_CONFIGURATION = 0xb1;    // Features = 0xc0
        static const u8 DEVICE_RESET = 0x08;
        static const u8 DOWNLOAD_MICROCODE = 0x92;
        static const u8 EXECUTE_DEVICE_DIAGNOSTIC = 0x90;
        static const u8 FLUSH_CACHE = 0xe7;
        static const u8 FLUSH_CACHE_EXT = 0xea;
        static const u8 GET_MEDIA_STATUS = 0xda;
        static const u8 IDENTIFY_DEVICE = 0xec;
        static const u8 IDENTIFY_PACKET_DEVICE = 0xa1;
        static const u8 IDLE = 0xe3;
        static const u8 IDLE_IMMEDIATE = 0xe1;
        static const u8 MEDIA_EJECT = 0xed;
        static const u8 MEDIA_LOCK = 0xde;
        static const u8 MEDIA_UNLOCK = 0xdf;
        static const u8 NOP = 0x00;
        static const u8 PACKET = 0xa0;
        static const u8 READ_BUFFER = 0xe4;
        static const u8 READ_DMA = 0xc8;
        static const u8 READ_DMA_EXT = 0x25;
        static const u8 READ_DMA_QUEUED = 0xc7;
        static const u8 READ_DMA_QUEUED_EXT = 0x26;
        static const u8 READ_LOG_EXT = 0x2f;
        static const u8 READ_MULTIPLE = 0xc4;
        static const u8 READ_MULTIPLE_EXT = 0x29;
        static const u8 READ_NATIVE_MAX_ADDRESS = 0xf8;
        static const u8 READ_NATIVE_MAX_ADDRESS_EXT = 0x27;
        static const u8 READ_SECTOR = 0x20;
        static const u8 READ_SECTOR_EXT = 0x24;
        static const u8 READ_VERIFY_SECTOR = 0x40;
        static const u8 READ_VERIFY_SECTOR_EXT = 0x42;
        static const u8 SECURITY_DISABLE_PASSWORD = 0xf6;
        static const u8 SECURITY_ERASE_PREPARE = 0xf3;
        static const u8 SECURITY_ERASE_UNIT = 0xf4;
        static const u8 SECURITY_FREEZE_LOCK = 0xf5;
        static const u8 SECURITY_SET_PASSWORD = 0xf1;
        static const u8 SECURITY_UNLOCK = 0xf2;
        static const u8 SEEK = 0x70;
        static const u8 SERVICE = 0xa2;
        static const u8 SET_FEATURES = 0xef;
        static const u8 SET_MAX = 0xf9;
        static const u8 SET_MAX_ADDRESS_EXT = 0x37;
        static const u8 SET_MULTIPLE_MODE = 0xc6;
        static const u8 SLEEP = 0xe6;
        static const u8 SMART = 0xb0;                   // Feature = 0xd9
        static const u8 STANDBY = 0xe2;
        static const u8 STANDBY_IMMEDIATE = 0xe0;
        static const u8 WRITE_BUFFER = 0xe8;
        static const u8 WRITE_DMA = 0xca;
        static const u8 WRITE_DMA_EXT = 0x35;
        static const u8 WRITE_DMA_QUEUED = 0xcc;
        static const u8 WRITE_DMA_QUEUED_EXT = 0x36;
        static const u8 WRITE_LOG_EXT = 0x3f;
        static const u8 WRITE_MULTIPLE = 0xc5;
        static const u8 WRITE_MULTIPLE_EXT = 0x39;
        static const u8 WRITE_SECTOR = 0x30;
        static const u8 WRITE_SECTOR_EXT = 0x34;
    };

    namespace DeviceIdentification
    {
        static const u8 GENERAL_CONFIGURATION = 0;
        static const u8 SPECIFIC_CONFIGURATION = 2;
        static const u8 SERIAL_NUMBER = 10; // -19;
        static const u8 FIRMWARE_REVISION = 23; // -26;
        static const u8 MODEL_NUMBER = 27;  // -46;
        static const u8 READ_WRITE_MULTIPLE_SUPPORT = 47;
        static const u8 CAPABILITIES = 49;  // -50;
        static const u8 FIELD_VALIDITY = 53;
        static const u8 MULTIPLE_SECTOR_SETTING = 59;
        static const u8 TOTAL_NUMBER_OF_USER_ADDRESSABLE_SECTORS = 60;  // -61;
        static const u8 MULTIWORD_DMA_TRANSFER = 63;
        static const u8 PIO_TRANSFER_MODES_SUPPORTED = 64;
        static const u8 MINIMUM_MULTIWORD_DMA_TRANSFER_CYCLE_TIME_PER_WORD = 65;
        static const u8 DEVICE_RECOMMENDED_MULTIWORD_DMA_CYCLE_TIME = 66;
        static const u8 MINIMUM_PIO_TRANSFER_CYCLE_TIME_WITHOUT_IORDY_FLOW_CONTROL = 67;
        static const u8 MINIMUM_PIO_TRANSFER_CYCLE_TIME_WITH_IORDY_FLOW_CONTROL = 68;
        static const u8 QUEUE_DEPTH = 75;
        static const u8 RESERVED_FOR_SERIAL_ATA = 76;   // -79;
        static const u8 MAJOR_VERSION_NUMBER = 80;
        static const u8 MINOR_VERSION_NUMBER = 81;
        static const u8 FEATURES_COMMAND_SETS_SUPPORTED = 82;   // -84;
        static const u8 FEATURES_COMMAND_SETS_ENABLED = 85; // -87;
        static const u8 ULTRA_DMA_MODES = 88;
        static const u8 TIME_REQUIRED_FOR_SECURITY_ERASE_UNIT_COMPLETION = 89;
        static const u8 TIME_REQUIRED_FOR_ENHANCED_SECURITY_ERASE_UNIT_COMPLETION = 90;
        static const u8 ADVANCED_POWER_MANAGEMENT_LEVEL_VALUE = 91;
        static const u8 MASTER_PASSWORD_REVISION_CODE = 92;
        static const u8 HARDWARE_CONFIGURATION_TEST_RESULTS = 93;
        static const u8 CURRENT_AUTOMATIC_ACOUSTIC_MANAGEMENT_VALUE = 94;
        static const u8 STREAM_MINIMUM_REQUEST_SIZE = 95;
        static const u8 STREAMING_TRANSFER_TIME_DMA = 96;
        static const u8 STREAMING_ACCESS_LATENCY_DMA_AND_PIO = 97;
        static const u8 STREAMING_PERFORMANCE_GRANULARITY = 98; // -99;
        static const u8 MAXIMUM_USER_LBA_FOR_48_BIT_ADDRESS_FEATURE_SET = 100;  // -103;
        static const u8 STREAMING_TRANSFER_TIME_PIO = 104;
        static const u8 PHYSICAL_SECTOR_SIZE_LOGICAL_SECTOR_SIZE = 106;
        static const u8 INTER_SEEK_DELAY_FOR_ISO_7779_STANDARD_ACOUSTIC_TESTING = 107;
        static const u8 WORLD_WIDE_NAME = 108;  // -111;
        static const u8 RESERVED_FOR_A_128_BIT_WORLD_WIDE_NAME = 112;   // -115;
        static const u8 RESERVED_FOR_TECHNICAL_REPORT = 116;
        static const u8 LOGICAL_SECTOR_SIZE = 118;  // -117;
        static const u8 REMOVABLE_MEDIA_STATUS_NOTIFICATION_FEATURE_SET_SUPPORT = 127;
        static const u8 SECURITY_STATUS = 128;
        static const u8 VENDOR_SPECIFIC = 129;  // -159;
        static const u8 CFA_POWER_MODE = 160;
        static const u8 CURRENT_MEDIA_SERIAL_NUMBER = 176;  // -205;
        static const u16 INTEGRITY_WORD = 255;
    };

    namespace PacketDeviceIdentification
    {
        static const u8 GENERAL_CONFIGURATION = 0;
        static const u8 SPECIFIC_CONFIGURATION = 2;
        static const u8 SERIAL_NUMBER = 10; // -19;
        static const u8 FIRMWARE_REVISION = 23; // -26;
        static const u8 MODEL_NUMBER = 27;  // -46;
        static const u8 CAPABILITIES = 49;  // -50
        static const u8 FIELD_VALIDITY = 53;
        static const u8 DMADIR = 62;
        static const u8 MULTIWORD_DMA_TRANSFER = 63;
        static const u8 PIO_TRANSFER_MODE_SUPPORTED = 64;
        static const u8 MINIMUM_MULTIWORD_DMA_TRANSFER_CYCLE_TIME_PER_WORD = 65;
        static const u8 DEVICE_RECOMMENDED_MULTIWORD_DMA_CYCLE_TIME = 66;
        static const u8 MINIMUM_PIO_TRANSFER_CYCLE_TIME_WITHOUT_FLOW_CONTROL = 67;
        static const u8 MINIMUM_PIO_TRANSFER_CYCLE_TIME_WITH_IORDY = 68;
        static const u8 PACKET_TO_BUS_RELEASE_TIME = 71;
        static const u8 SERVICE_TO_BUS_RELEASE_TIME = 72;
        static const u8 QUEUE_DEPTH = 75;
        static const u8 MAJOR_REVISION_NUMBER = 80;
        static const u8 MINOR_REVISION_NUMBER = 81;
        static const u8 FEATURES_COMMAND_SETS_SUPPORTED = 82;   // -84;
        static const u8 FEATURES_COMMAND_SETS_ENABLED = 85; // -87;
        static const u8 ULTRA_DMA_MODES = 88;
        static const u8 TIME_REQUIRED_FOR_SECURITY_ERASE_UNIT_COMPLETION = 89;
        static const u8 TIME_REQUIRED_FOR_ENHANCED_SECURITY_ERASE_UNIT_COMPLETION = 90;
        static const u8 HARDWARE_RESET_RESULTS = 93;
        static const u8 CURRENT_AUTOMATIC_ACOUSTIC_MANAGEMENT_VALUE = 94;
        static const u8 ATAPI_BYTE_COUNT_0_BEHAVIOR = 125;
        static const u8 REMOVABLE_MEDIA_STATUS_NOTIFICATION_FEATURE_SET_SUPPORT = 127;
        static const u8 SECURITY_STATUS = 128;
        static const u16 INTEGRITY_WORD = 255;
    };

    namespace PacketCommand
    {
        // ATAPI CD Commands
        static const u8 BLANK = 0xA1;
        static const u8 CLOSE_TRACK_SESSION = 0x5B;
        static const u8 FORMAT_UNIT = 0x04;
        static const u8 INQUIRY = 0x12;
        static const u8 LOAD_UNLOAD_CD = 0xA6;
        static const u8 MECHANISM_STATUS = 0xBD;
        static const u8 MODE_SELECT_10 = 0x55;
        static const u8 MODE_SENSE_10 = 0x5A;
        static const u8 PAUSE_RESUME = 0x4B;
        static const u8 PLAY_AUDIO_10 = 0x45;
        static const u8 PLAY_AUDIO_12 = 0xA5;
        static const u8 PLAY_AUDIO_MSF = 0x47;
        static const u8 PLAY_CD = 0xBC;
        static const u8 PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E;
        static const u8 READ_10 = 0x28;
        static const u8 READ_12 = 0xA8;
        static const u8 READ_BUFFER_CAPACITY = 0x5C;
        static const u8 READ_CD = 0xBE;
        static const u8 READ_CD_MSF = 0xB9;
        static const u8 READ_CD_RECORDED_CAPACITY = 0x25;
        static const u8 READ_DISC_INFORMATION = 0x51;
        static const u8 READ_HEADER = 0x44;
        static const u8 READ_MASTER_CUE = 0x59;
        static const u8 READ_SUB_CHANNEL = 0x42;
        static const u8 READ_TOC = 0x43;
        static const u8 READ_TRACK_INFORMATION = 0x52;
        static const u8 REPAIR_TRACK = 0x58;
        static const u8 REQUEST_SENSE = 0x03;
        static const u8 RESERVE_TRACK = 0x53;
        static const u8 SCAN = 0xBA;
        static const u8 SEEK = 0x2B;
        static const u8 SEND_CUE_SHEET = 0x5D;
        static const u8 SEND_OPC_INFORMATION = 0x54;
        static const u8 SET_CD_SPEED = 0xBB;
        static const u8 START_STOP_UNIT = 0x1B;
        static const u8 STOP_PLAY_SCAN = 0x4E;
        static const u8 SYNCHRONIZE_CACHE = 0x35;
        static const u8 TEST_UNIT_READY = 0x00;
        static const u8 WRITE_10 = 0x2A;
        static const u8 WRITE_12 = 0xAA;
    };

}

#endif // NINTENDO_ES_KERNEL_ATA_H_INCLUDED
