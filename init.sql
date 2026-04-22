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
    category VARCHAR(50) COMMENT '题目类型'
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

-- 7. 创建数据库用户

-- 7.1 管理员用户（全权限）
-- 使用 '%' 允许从任何主机连接（Docker 容器间通信需要）
CREATE USER IF NOT EXISTS 'oj_admin' @'%' IDENTIFIED BY '090800';

GRANT SELECT, INSERT , UPDATE, DELETE ON OJ.* TO 'oj_admin' @'%';

-- 7.2 普通用户（受限权限）
-- 说明：所有平台用户复用此账号连接数据库，行级隔离由应用程序控制
CREATE USER IF NOT EXISTS 'oj_user' @'%' IDENTIFIED BY 'user123';

-- 题目表：只读
GRANT SELECT ON OJ.problems TO 'oj_user' @'%';

-- 用户表：可插入新用户、查询和修改自己的账户名、密码
-- 注意：应用程序通过 WHERE id = current_user_id 实现行级隔离
GRANT SELECT, INSERT , UPDATE ON OJ.users TO 'oj_user' @'%';

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
        test_data_path,
        category
    )
SELECT 'A+B Problem', 'Calculate the sum of two integers.', 1000, 128, 'data/1', '模拟'
WHERE
    NOT EXISTS (
        SELECT 1
        FROM problems
        WHERE
            id = 1
    );

-- 9.2 动态规划题目
INSERT INTO
    problems (
        title,
        description,
        time_limit,
        memory_limit,
        test_data_path,
        category
    )
SELECT '爬楼梯', '假设你正在爬楼梯。需要 n 阶你才能到达楼顶。每次你可以爬 1 或 2 个台阶。你有多少种不同的方法可以爬到楼顶呢？\n\n输入：一个整数 n，表示楼梯的阶数（1 <= n <= 45）\n输出：爬到楼顶的不同方法数', 1000, 128, 'data/2', '动态规划'
WHERE
    NOT EXISTS (
        SELECT 1
        FROM problems
        WHERE
            id = 2
    );

-- 9.3 贪心算法题目
INSERT INTO
    problems (
        title,
        description,
        time_limit,
        memory_limit,
        test_data_path,
        category
    )
SELECT '买卖股票的最佳时机', '给定一个数组 prices，它的第 i 个元素 prices[i] 表示一支给定股票第 i 天的价格。\n你只能选择某一天买入这只股票，并选择在未来的某一个不同的日子卖出该股票。设计一个算法来计算你所能获取的最大利润。\n\n输入：第一行一个整数 n，表示天数；第二行 n 个整数，表示每天的股票价格\n输出：最大利润（如果无法获取利润，输出 0）', 1000, 128, 'data/3', '贪心'
WHERE
    NOT EXISTS (
        SELECT 1
        FROM problems
        WHERE
            id = 3
    );

-- 9.4 搜索题目
INSERT INTO
    problems (
        title,
        description,
        time_limit,
        memory_limit,
        test_data_path,
        category
    )
SELECT '迷宫探索', '给定一个 n x m 的迷宫，其中 0 表示可以通过的路，1 表示墙。从左上角 (0,0) 出发，能否到达右下角 (n-1,m-1)？\n\n输入：第一行两个整数 n, m；接下来 n 行，每行 m 个整数（0 或 1）\n输出：能到达输出 "Yes"，否则输出 "No"', 2000, 256, 'data/4', '搜索'
WHERE
    NOT EXISTS (
        SELECT 1
        FROM problems
        WHERE
            id = 4
    );

-- 9.5 数学题目
INSERT INTO
    problems (
        title,
        description,
        time_limit,
        memory_limit,
        test_data_path,
        category
    )
SELECT '素数判断', '给定一个正整数 n，判断它是否为素数。\n\n输入：一个整数 n（2 <= n <= 10^9）\n输出：是素数输出 "Yes"，否则输出 "No"', 1000, 128, 'data/5', '数学'
WHERE
    NOT EXISTS (
        SELECT 1
        FROM problems
        WHERE
            id = 5
    );

-- 9.6 字符串题目
INSERT INTO
    problems (
        title,
        description,
        time_limit,
        memory_limit,
        test_data_path,
        category
    )
SELECT '回文串判断', '给定一个字符串，判断它是否是回文串（正读反读都相同的字符串）。\n\n输入：一个字符串（长度不超过 1000，只包含小写字母）\n输出：是回文串输出 "Yes"，否则输出 "No"', 1000, 128, 'data/6', '字符串'
WHERE
    NOT EXISTS (
        SELECT 1
        FROM problems
        WHERE
            id = 6
    );

-- 9.7 数据结构题目
INSERT INTO
    problems (
        title,
        description,
        time_limit,
        memory_limit,
        test_data_path,
        category
    )
SELECT '有效的括号', '给定一个只包括圆括号、花括号、方括号的字符串 s，判断字符串是否有效。\n有效字符串需满足：左括号必须用相同类型的右括号闭合，左括号必须以正确的顺序闭合。\n\n输入：一个字符串 s（长度不超过 10000，只包含 (){}[]）\n输出：有效输出 "Yes"，否则输出 "No"', 1000, 128, 'data/7', '数据结构'
WHERE
    NOT EXISTS (
        SELECT 1
        FROM problems
        WHERE
            id = 7
    );

-- 9.8 图论题目
INSERT INTO
    problems (
        title,
        description,
        time_limit,
        memory_limit,
        test_data_path,
        category
    )
SELECT '最短路径', '给定一个无向图，包含 n 个节点和 m 条边，每条边有一个正权值。求节点 1 到节点 n 的最短路径长度。\n\n输入：第一行两个整数 n, m；接下来 m 行，每行三个整数 u, v, w，表示节点 u 和 v 之间有一条权值为 w 的边\n输出：一个整数，表示最短路径长度（如果无法到达，输出 -1）', 2000, 256, 'data/8', '图论'
WHERE
    NOT EXISTS (
        SELECT 1
        FROM problems
        WHERE
            id = 8
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

SELECT CONCAT(
        'Sample platform user: test_user / 123456'
    ) AS Sample;