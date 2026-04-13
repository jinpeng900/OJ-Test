-- ==========================================================
-- OJ 系统环境初始化脚本 (仅用于外部测试/迁移)
-- 使用说明:
-- 1. 确保已安装 MySQL
-- 2. 命令行执行: mysql -u root -p < init.sql
-- ==========================================================

-- 1. 创建数据库
CREATE DATABASE IF NOT EXISTS OJ CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- 2. 切换到 OJ 数据库
USE OJ;

-- 3. 创建题目表
CREATE TABLE IF NOT EXISTS problems (
    id INT AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    description TEXT,
    time_limit INT COMMENT '时间限制(ms)',
    memory_limit INT COMMENT '内存限制(MB)',
    test_data_path VARCHAR(256) COMMENT '宿主机测试数据的存放路径',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE = InnoDB;

-- 4. 创建用户表
-- 存储平台用户信息（不是数据库用户）
CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT '用户ID，内部使用',
    account VARCHAR(50) UNIQUE NOT NULL COMMENT '登录账号，唯一标识',
    password_hash VARCHAR(255) NOT NULL COMMENT '密码哈希（SHA256）',
    submission_count INT DEFAULT 0 COMMENT '提交题目数量',
    solved_count INT DEFAULT 0 COMMENT '解决题目数量',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '注册时间',
    last_login TIMESTAMP NULL DEFAULT NULL COMMENT '最后登录时间',
    -- 索引
    INDEX idx_account (account),
    INDEX idx_created_at (created_at)
) ENGINE = InnoDB COMMENT = '平台用户表';

-- 5. 创建提交记录表
CREATE TABLE IF NOT EXISTS submissions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL COMMENT '提交用户ID',
    problem_id INT NOT NULL COMMENT '题目ID',
    code TEXT COMMENT '提交的代码',
    status ENUM(
        'Pending',
        'AC',
        'WA',
        'TLE',
        'MLE',
        'RE',
        'CE'
    ) DEFAULT 'Pending' COMMENT '评测状态',
    submit_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users (id),
    FOREIGN KEY (problem_id) REFERENCES problems (id),
    INDEX idx_user_id (user_id),
    INDEX idx_problem_id (problem_id)
) ENGINE = InnoDB;

-- 6. 配置 MySQL 密码策略 (针对部分高版本 MySQL 环境)
SET GLOBAL validate_password.policy = LOW;

SET GLOBAL validate_password.length = 6;

-- 7. 创建数据库用户

-- 7.1 管理员用户（全权限）
CREATE USER IF NOT EXISTS 'oj_admin' @'localhost' IDENTIFIED BY '090800';

GRANT
SELECT,
INSERT
,
UPDATE,
DELETE ON OJ.* TO 'oj_admin' @'localhost';

-- 7.2 普通用户（受限权限）
-- 说明：所有平台用户复用此账号连接数据库，行级隔离由应用程序控制
CREATE USER IF NOT EXISTS 'oj_user' @'%' IDENTIFIED BY 'user123';

-- 题目表：只读
GRANT SELECT ON OJ.problems TO 'oj_user' @'%';

-- 用户表：可查询和修改自己的账户名、密码
-- 注意：应用程序通过 WHERE id = current_user_id 实现行级隔离
GRANT
SELECT,
UPDATE (account, password_hash) ON OJ.users TO 'oj_user' @'%';

-- 提交记录表：可插入新提交，查询自己的历史
GRANT SELECT, INSERT ON OJ.submissions TO 'oj_user' @'%';

-- 8. 刷新权限
FLUSH PRIVILEGES;

-- 9. 插入示例数据

-- 9.1 示例题目
INSERT INTO
    problems (
        title,
        description,
        time_limit,
        memory_limit,
        test_data_path
    )
SELECT 'A+B Problem', 'Calculate the sum of two integers.', 1000, 128, '/home/oj/data/1'
WHERE
    NOT EXISTS (
        SELECT 1
        FROM problems
        WHERE
            id = 1
    );

-- 9.2 示例平台用户（密码: 123456）
-- SHA256('123456') = '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92'
INSERT INTO
    users (
        account,
        password_hash,
        submission_count,
        solved_count
    )
SELECT 'test_user', '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 0, 0
WHERE
    NOT EXISTS (
        SELECT 1
        FROM users
        WHERE
            account = 'test_user'
    );

-- 完成提示
SELECT 'Database OJ initialized successfully!' AS Result;

SELECT CONCAT(
        'Users created: oj_admin (full access), oj_user (limited access)'
    ) AS Info;

SELECT CONCAT( 'Sample platform user: test_user / 123456' ) AS Sample;