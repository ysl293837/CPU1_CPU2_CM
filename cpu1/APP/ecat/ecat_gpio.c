#include "driverlib.h"
#include "ecat_gpio.h"

/* 1. 配置 F28388D 开发板的 EtherCAT 专用引脚 */
void ECAT_GPIO_init(void)
{
    /* 2. 配置 PHY 时钟和复位 */
    GPIO_setPinConfig(GPIO_48_ESC_PHY_CLK);               // 将 GPIO48 复用为 EtherCAT PHY 时钟输出
    GPIO_setPinConfig(GPIO_76_ESC_PHY_RESETN);            // 将 GPIO76 复用为 EtherCAT PHY 复位信号

    /* 3. 配置 EEPROM 的 I2C 引脚 */
    GPIO_setPinConfig(GPIO_40_ESC_I2C_SDA);               // 将 GPIO40 复用为 ESC EEPROM 数据线
    GPIO_setQualificationMode(40U, GPIO_QUAL_ASYNC);      // I2C 数据线使用异步采样
    GPIO_setPadConfig(40U, GPIO_PIN_TYPE_PULLUP);         // 给 I2C 数据线配置上拉
    GPIO_setPinConfig(GPIO_41_ESC_I2C_SCL);               // 将 GPIO41 复用为 ESC EEPROM 时钟线
    GPIO_setQualificationMode(41U, GPIO_QUAL_ASYNC);      // I2C 时钟线使用异步采样
    GPIO_setPadConfig(41U, GPIO_PIN_TYPE_PULLUP);         // 给 I2C 时钟线配置上拉

    /* 4. 配置 EtherCAT 端口 0 的发送数据线 */
    GPIO_setPinConfig(GPIO_87_ESC_TX0_DATA0);             // 配置端口 0 发送数据位 0
    GPIO_setQualificationMode(87U, GPIO_QUAL_ASYNC);      // 发送数据位 0 使用异步路径
    GPIO_setPinConfig(GPIO_88_ESC_TX0_DATA1);             // 配置端口 0 发送数据位 1
    GPIO_setQualificationMode(88U, GPIO_QUAL_ASYNC);      // 发送数据位 1 使用异步路径
    GPIO_setPinConfig(GPIO_89_ESC_TX0_DATA2);             // 配置端口 0 发送数据位 2
    GPIO_setQualificationMode(89U, GPIO_QUAL_ASYNC);      // 发送数据位 2 使用异步路径
    GPIO_setPinConfig(GPIO_90_ESC_TX0_DATA3);             // 配置端口 0 发送数据位 3
    GPIO_setQualificationMode(90U, GPIO_QUAL_ASYNC);      // 发送数据位 3 使用异步路径

    /* 5. 配置 EtherCAT 端口 0 的接收数据线 */
    GPIO_setPinConfig(GPIO_80_ESC_RX0_DATA0);             // 配置端口 0 接收数据位 0
    GPIO_setQualificationMode(80U, GPIO_QUAL_ASYNC);      // 接收数据位 0 使用异步采样
    GPIO_setPinConfig(GPIO_81_ESC_RX0_DATA1);             // 配置端口 0 接收数据位 1
    GPIO_setQualificationMode(81U, GPIO_QUAL_ASYNC);      // 接收数据位 1 使用异步采样
    GPIO_setPinConfig(GPIO_82_ESC_RX0_DATA2);             // 配置端口 0 接收数据位 2
    GPIO_setQualificationMode(82U, GPIO_QUAL_ASYNC);      // 接收数据位 2 使用异步采样
    GPIO_setPinConfig(GPIO_83_ESC_RX0_DATA3);             // 配置端口 0 接收数据位 3
    GPIO_setQualificationMode(83U, GPIO_QUAL_ASYNC);      // 接收数据位 3 使用异步采样

    /* 6. 配置 EtherCAT 端口 0 的控制线和时钟线 */
    GPIO_setPinConfig(GPIO_84_ESC_TX0_ENA);               // 配置端口 0 发送使能信号
    GPIO_setQualificationMode(84U, GPIO_QUAL_ASYNC);      // 发送使能信号使用异步路径
    GPIO_setPinConfig(GPIO_78_ESC_RX0_DV);                // 配置端口 0 接收数据有效信号
    GPIO_setQualificationMode(78U, GPIO_QUAL_ASYNC);      // 接收数据有效信号使用异步采样
    GPIO_setPinConfig(GPIO_79_ESC_RX0_ERR);               // 配置端口 0 接收错误信号
    GPIO_setQualificationMode(79U, GPIO_QUAL_ASYNC);      // 接收错误信号使用异步采样
    GPIO_setPinConfig(GPIO_85_ESC_TX0_CLK);               // 配置端口 0 发送时钟信号
    GPIO_setQualificationMode(85U, GPIO_QUAL_ASYNC);      // 发送时钟信号使用异步路径
    GPIO_setPinConfig(GPIO_77_ESC_RX0_CLK);               // 配置端口 0 接收时钟信号
    GPIO_setQualificationMode(77U, GPIO_QUAL_ASYNC);      // 接收时钟信号使用异步采样

    /* 7. 配置 EtherCAT 端口 1 的发送数据线 */
    GPIO_setPinConfig(GPIO_75_ESC_TX1_DATA0);             // 配置端口 1 发送数据位 0
    GPIO_setQualificationMode(75U, GPIO_QUAL_ASYNC);      // 发送数据位 0 使用异步路径
    GPIO_setPinConfig(GPIO_74_ESC_TX1_DATA1);             // 配置端口 1 发送数据位 1
    GPIO_setQualificationMode(74U, GPIO_QUAL_ASYNC);      // 发送数据位 1 使用异步路径
    GPIO_setPinConfig(GPIO_73_ESC_TX1_DATA2);             // 配置端口 1 发送数据位 2
    GPIO_setQualificationMode(73U, GPIO_QUAL_ASYNC);      // 发送数据位 2 使用异步路径
    GPIO_setPinConfig(GPIO_72_ESC_TX1_DATA3);             // 配置端口 1 发送数据位 3
    GPIO_setQualificationMode(72U, GPIO_QUAL_ASYNC);      // 发送数据位 3 使用异步路径

    /* 8. 配置 EtherCAT 端口 1 的接收数据线 */
    GPIO_setPinConfig(GPIO_63_ESC_RX1_DATA0);             // 配置端口 1 接收数据位 0
    GPIO_setQualificationMode(63U, GPIO_QUAL_ASYNC);      // 接收数据位 0 使用异步采样
    GPIO_setPinConfig(GPIO_64_ESC_RX1_DATA1);             // 配置端口 1 接收数据位 1
    GPIO_setQualificationMode(64U, GPIO_QUAL_ASYNC);      // 接收数据位 1 使用异步采样
    GPIO_setPinConfig(GPIO_65_ESC_RX1_DATA2);             // 配置端口 1 接收数据位 2
    GPIO_setQualificationMode(65U, GPIO_QUAL_ASYNC);      // 接收数据位 2 使用异步采样
    GPIO_setPinConfig(GPIO_66_ESC_RX1_DATA3);             // 配置端口 1 接收数据位 3
    GPIO_setQualificationMode(66U, GPIO_QUAL_ASYNC);      // 接收数据位 3 使用异步采样

    /* 9. 配置 EtherCAT 端口 1 的控制线和时钟线 */
    GPIO_setPinConfig(GPIO_45_ESC_TX1_ENA);               // 配置端口 1 发送使能信号
    GPIO_setQualificationMode(45U, GPIO_QUAL_ASYNC);      // 发送使能信号使用异步路径
    GPIO_setPinConfig(GPIO_70_ESC_RX1_DV);                // 配置端口 1 接收数据有效信号
    GPIO_setQualificationMode(70U, GPIO_QUAL_ASYNC);      // 接收数据有效信号使用异步采样
    GPIO_setPinConfig(GPIO_71_ESC_RX1_ERR);               // 配置端口 1 接收错误信号
    GPIO_setQualificationMode(71U, GPIO_QUAL_ASYNC);      // 接收错误信号使用异步采样
    GPIO_setPinConfig(GPIO_44_ESC_TX1_CLK);               // 配置端口 1 发送时钟信号
    GPIO_setQualificationMode(44U, GPIO_QUAL_ASYNC);      // 发送时钟信号使用异步路径
    GPIO_setPinConfig(GPIO_69_ESC_RX1_CLK);               // 配置端口 1 接收时钟信号
    GPIO_setQualificationMode(69U, GPIO_QUAL_ASYNC);      // 接收时钟信号使用异步采样

    /* 10. 配置 PHY 管理接口和链路状态输入 */
    GPIO_setPinConfig(GPIO_26_ESC_MDIO_CLK);              // 配置 MDIO 管理时钟
    GPIO_setQualificationMode(26U, GPIO_QUAL_ASYNC);      // MDIO 时钟使用异步路径
    GPIO_setPinConfig(GPIO_27_ESC_MDIO_DATA);             // 配置 MDIO 管理数据线
    GPIO_setQualificationMode(27U, GPIO_QUAL_ASYNC);      // MDIO 数据线使用异步采样
    GPIO_setPinConfig(GPIO_86_ESC_PHY0_LINKSTATUS);       // 配置端口 0 PHY 链路状态输入
    GPIO_setQualificationMode(86U, GPIO_QUAL_ASYNC);      // 链路状态使用异步采样
    GPIO_setPadConfig(86U, GPIO_PIN_TYPE_PULLUP);         // 为端口 0 链路状态配置上拉
    GPIO_setPinConfig(GPIO_68_ESC_PHY1_LINKSTATUS);       // 配置端口 1 PHY 链路状态输入
    GPIO_setQualificationMode(68U, GPIO_QUAL_ASYNC);      // 链路状态使用异步采样
    GPIO_setPadConfig(68U, GPIO_PIN_TYPE_PULLUP);         // 为端口 1 链路状态配置上拉

    /* 11. 配置 EtherCAT 状态指示灯 */
    GPIO_setPinConfig(GPIO_58_ESC_LED_LINK0_ACTIVE);      // 配置端口 0 链路活动指示灯
    GPIO_setQualificationMode(58U, GPIO_QUAL_ASYNC);      // 指示灯信号使用异步路径
    GPIO_setPinConfig(GPIO_59_ESC_LED_LINK1_ACTIVE);      // 配置端口 1 链路活动指示灯
    GPIO_setQualificationMode(59U, GPIO_QUAL_ASYNC);      // 指示灯信号使用异步路径
    GPIO_setPinConfig(GPIO_60_ESC_LED_ERR);               // 配置 EtherCAT 错误指示灯
    GPIO_setQualificationMode(60U, GPIO_QUAL_ASYNC);      // 指示灯信号使用异步路径
    GPIO_setPinConfig(GPIO_61_ESC_LED_RUN);               // 配置 EtherCAT 运行指示灯
    GPIO_setQualificationMode(61U, GPIO_QUAL_ASYNC);      // 指示灯信号使用异步路径
    GPIO_setPinConfig(GPIO_62_ESC_LED_STATE_RUN);         // 配置 EtherCAT 状态运行指示灯
    GPIO_setQualificationMode(62U, GPIO_QUAL_ASYNC);      // 指示灯信号使用异步路径

    /* 12. 配置 EtherCAT 同步输出 */
    GPIO_setPinConfig(GPIO_34_ESC_SYNC0);                 // 将 GPIO34 复用为 ESC SYNC0 输出
    GPIO_setDirectionMode(34U, GPIO_DIR_MODE_OUT);        // 将 SYNC0 设置为输出方向
    GPIO_setPinConfig(GPIO_35_ESC_SYNC1);                 // 将 GPIO35 复用为 ESC SYNC1 输出
    GPIO_setDirectionMode(35U, GPIO_DIR_MODE_OUT);        // 将 SYNC1 设置为输出方向
}
