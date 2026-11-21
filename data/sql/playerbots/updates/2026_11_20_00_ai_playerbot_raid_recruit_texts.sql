-- #########################################################
-- Playerbots - Replies of random bots to raid recruitment
-- messages in the General and LookingForGroup channels
-- English only: the locale columns keep their empty default
-- and fall back to the English text
-- #########################################################

DELETE FROM ai_playerbot_texts WHERE name IN (
    'raid_recruit_reply_tank',
    'raid_recruit_reply_heal',
    'raid_recruit_reply_dps',
    'raid_recruit_reply_class_tank',
    'raid_recruit_reply_class_heal',
    'raid_recruit_reply_class_dps'
);

DELETE FROM ai_playerbot_texts_chance WHERE name IN (
    'raid_recruit_reply_tank',
    'raid_recruit_reply_heal',
    'raid_recruit_reply_dps',
    'raid_recruit_reply_class_tank',
    'raid_recruit_reply_class_heal',
    'raid_recruit_reply_class_dps'
);

INSERT INTO ai_playerbot_texts (id, name, text, say_type, reply_type) VALUES
    (1945, 'raid_recruit_reply_tank', 'tank here, %gs gs, inv', 0, 0),
    (1946, 'raid_recruit_reply_tank', 'tank %gs, can come', 0, 0),
    (1947, 'raid_recruit_reply_tank', 'i can tank, %gs gs', 0, 0),
    (1948, 'raid_recruit_reply_heal', 'healer here, %gs gs, inv', 0, 0),
    (1949, 'raid_recruit_reply_heal', 'heal %gs, inv me', 0, 0),
    (1950, 'raid_recruit_reply_heal', 'i can heal, %gs gs', 0, 0),
    (1951, 'raid_recruit_reply_dps', 'dps here, %gs gs, inv', 0, 0),
    (1952, 'raid_recruit_reply_dps', 'dps %gs, can come', 0, 0),
    (1953, 'raid_recruit_reply_dps', '%gs dps, inv pls', 0, 0),
    (1954, 'raid_recruit_reply_class_tank', '%class tank here, %gs gs, inv', 0, 0),
    (1955, 'raid_recruit_reply_class_tank', '%class tank %gs, can come', 0, 0),
    (1956, 'raid_recruit_reply_class_tank', '%gs %class tank, inv me', 0, 0),
    (1957, 'raid_recruit_reply_class_heal', '%class healer here, %gs gs, inv', 0, 0),
    (1958, 'raid_recruit_reply_class_heal', '%class heal %gs, inv me', 0, 0),
    (1959, 'raid_recruit_reply_class_heal', '%gs %class healer, can come', 0, 0),
    (1960, 'raid_recruit_reply_class_dps', '%class dps here, %gs gs, inv', 0, 0),
    (1961, 'raid_recruit_reply_class_dps', '%class %gs, can come', 0, 0),
    (1962, 'raid_recruit_reply_class_dps', '%gs %class dps, inv pls', 0, 0);

INSERT INTO ai_playerbot_texts_chance (name, probability) VALUES
    ('raid_recruit_reply_tank', 100),
    ('raid_recruit_reply_heal', 100),
    ('raid_recruit_reply_dps', 100),
    ('raid_recruit_reply_class_tank', 100),
    ('raid_recruit_reply_class_heal', 100),
    ('raid_recruit_reply_class_dps', 100);
