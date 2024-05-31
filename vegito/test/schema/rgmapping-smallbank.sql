CREATE PROPERTY GRAPH smallbank
    VERTEX TABLES (
        Accounts
            KEY ( custid )
            LABEL Account PROPERTIES ( custid, name),
        Savings
            KEY ( custid )
            LABEL Saving PROPERTIES ( custid, bal),
        Checking
            KEY ( custid )
            LABEL Checking PROPERTIES ( custid, bal)
    )
    EDGE TABLES (
        Savings
            SOURCE KEY ( custid ) REFERENCES Accounts
            DESTINATION KEY ( custid ) REFERENCES Savings
            LABEL saving_account NO PROPERTIES,
        Checking
            SOURCE KEY ( custid ) REFERENCES Accounts
            DESTINATION KEY ( custid ) REFERENCES Checking
            LABEL checking_account NO PROPERTIES
    )
