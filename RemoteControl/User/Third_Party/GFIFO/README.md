/**
 * @file README.md
 * @brief GFIFO 环形缓冲库的引入记录（Third_Party 只读区）
 *
 * - 来源：本地仓库 D:/a2/Warehouse/GFIFO，提交 2e6bad6（2026-10-31）
 * - 版本：库内文档标注 1.0，四个文件整体拷入，未做任何修改
 * - 文件：gfifo.c / gfifo.h / gfifo_cfg.h / gfifo_port.h
 * - 配置：沿用库默认值，仅依赖 C99 与 string.h（GFIFO_CFG_USE_LIBC = 1）
 * - 用法：SPSC 场景传 GFIFO_FLAG_NONE，中断只写、任务只读，库内部无锁
 *
 * @note 本目录属于 Third_Party：不改造、不格式化、不在此添加任何业务逻辑。
 *       升级时用上游四个文件整体覆盖，本地差异应为零。
 */
