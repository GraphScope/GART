CREATE DATABASE IF NOT EXISTS financial_transactions;
USE financial_transactions;

CREATE TABLE Persons (
            id BIGINT NOT NULL,
            name VARCHAR(255),
            company_id BIGINT,
            PRIMARY KEY (id),
            FOREIGN KEY (company_id) REFERENCES Companies(id)
        );


CREATE TABLE Companies (
            id BIGINT NOT NULL,
            name VARCHAR(255),
            PRIMARY KEY (id)
        );

CREATE TABLE Accounts (
            number BIGINT NOT NULL,
            account_type VARCHAR(255),
            person_id BIGINT,
            company_id BIGINT,
            PRIMARY KEY (number),
            FOREIGN KEY (person_id) REFERENCES Persons(id),
            FOREIGN KEY (company_id) REFERENCES Companies(id)
        );

CREATE TABLE Transactions (
            id BIGINT NOT NULL,
            from_account BIGINT,
            to_account BIGINT,
            date VARCHAR(255),
            amount DOUBLE,
            PRIMARY KEY (id),
            FOREIGN KEY (from_account) REFERENCES Accounts(number),
            FOREIGN KEY (to_account) REFERENCES Accounts(number)
        );