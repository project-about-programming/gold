PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS app_settings (
    setting_key TEXT PRIMARY KEY,
    setting_value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY,
    full_name TEXT NOT NULL,
    username TEXT NOT NULL UNIQUE,
    email TEXT NOT NULL,
    role TEXT NOT NULL,
    password_hash TEXT NOT NULL,
    status TEXT NOT NULL,
    last_login TEXT,
    created_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS roles (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    description TEXT NOT NULL,
    default_start_page TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS role_permissions (
    role_id INTEGER NOT NULL,
    permission TEXT NOT NULL,
    scope TEXT NOT NULL DEFAULT 'none',
    PRIMARY KEY (role_id, permission),
    FOREIGN KEY (role_id) REFERENCES roles(id)
);

CREATE TABLE IF NOT EXISTS clients (
    id INTEGER PRIMARY KEY,
    company_name TEXT NOT NULL,
    contact_name TEXT NOT NULL,
    email TEXT NOT NULL,
    phone TEXT NOT NULL,
    segment TEXT NOT NULL,
    region TEXT NOT NULL,
    is_active INTEGER NOT NULL DEFAULT 1,
    last_order_date TEXT,
    lifetime_value REAL NOT NULL DEFAULT 0,
    account_manager TEXT NOT NULL,
    manager_employee_id INTEGER
);

CREATE TABLE IF NOT EXISTS products (
    id INTEGER PRIMARY KEY,
    sku TEXT NOT NULL UNIQUE,
    name TEXT NOT NULL,
    category TEXT NOT NULL,
    stock INTEGER NOT NULL,
    reorder_level INTEGER NOT NULL,
    price REAL NOT NULL,
    cost REAL NOT NULL DEFAULT 0,
    discount_percent REAL NOT NULL DEFAULT 0,
    status TEXT NOT NULL,
    supplier TEXT NOT NULL,
    margin_percent REAL NOT NULL,
    expiration_date TEXT NOT NULL DEFAULT '2026-12-31',
    batch_code TEXT NOT NULL DEFAULT 'BATCH-DEMO'
);

CREATE TABLE IF NOT EXISTS employees (
    id INTEGER PRIMARY KEY,
    full_name TEXT NOT NULL,
    role TEXT NOT NULL,
    region TEXT NOT NULL,
    status TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS orders (
    id INTEGER PRIMARY KEY,
    order_code TEXT NOT NULL UNIQUE,
    client_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    quantity INTEGER NOT NULL,
    total_price REAL NOT NULL,
    order_date TEXT NOT NULL,
    status TEXT NOT NULL,
    priority TEXT NOT NULL,
    FOREIGN KEY (client_id) REFERENCES clients(id),
    FOREIGN KEY (product_id) REFERENCES products(id)
);

CREATE TABLE IF NOT EXISTS sales (
    id INTEGER PRIMARY KEY,
    sale_code TEXT NOT NULL UNIQUE,
    employee_id INTEGER NOT NULL,
    client_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    channel TEXT NOT NULL,
    quantity INTEGER NOT NULL DEFAULT 1,
    unit_price REAL NOT NULL DEFAULT 0,
    revenue REAL NOT NULL,
    total_amount REAL NOT NULL DEFAULT 0,
    margin_percent REAL NOT NULL,
    sale_date TEXT NOT NULL,
    status TEXT NOT NULL,
    payment_method TEXT NOT NULL DEFAULT 'Card',
    cancellation_reason TEXT,
    FOREIGN KEY (employee_id) REFERENCES employees(id),
    FOREIGN KEY (client_id) REFERENCES clients(id),
    FOREIGN KEY (product_id) REFERENCES products(id)
);

CREATE TABLE IF NOT EXISTS pipeline_deals (
    id INTEGER PRIMARY KEY,
    lead_name TEXT NOT NULL,
    client_id INTEGER,
    manager_id INTEGER,
    stage TEXT NOT NULL,
    value REAL NOT NULL DEFAULT 0,
    expected_close_date TEXT,
    status TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    FOREIGN KEY (client_id) REFERENCES clients(id),
    FOREIGN KEY (manager_id) REFERENCES employees(id)
);

CREATE TABLE IF NOT EXISTS tasks (
    id INTEGER PRIMARY KEY,
    title TEXT NOT NULL,
    assigned_to TEXT NOT NULL,
    due_date TEXT NOT NULL,
    status TEXT NOT NULL,
    priority TEXT NOT NULL,
    related_client_id INTEGER,
    created_at TEXT NOT NULL,
    FOREIGN KEY (related_client_id) REFERENCES clients(id)
);

CREATE TABLE IF NOT EXISTS audit_log (
    id INTEGER PRIMARY KEY,
    entity_type TEXT NOT NULL,
    entity_id INTEGER NOT NULL DEFAULT 0,
    action TEXT NOT NULL,
    user_name TEXT NOT NULL,
    details TEXT NOT NULL,
    created_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_orders_date ON orders(order_date);
CREATE INDEX IF NOT EXISTS idx_orders_status ON orders(status);
CREATE INDEX IF NOT EXISTS idx_sales_date ON sales(sale_date);
CREATE INDEX IF NOT EXISTS idx_sales_status ON sales(status);
CREATE INDEX IF NOT EXISTS idx_pipeline_stage ON pipeline_deals(stage);
CREATE INDEX IF NOT EXISTS idx_tasks_due ON tasks(due_date, status);
CREATE INDEX IF NOT EXISTS idx_audit_created ON audit_log(created_at);
